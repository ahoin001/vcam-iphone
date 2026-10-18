#include "FrameBus.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstring>

namespace vcam {
namespace {

uint32_t MappingBytes() {
    return static_cast<uint32_t>(sizeof(FrameBusHeader) + static_cast<size_t>(FrameByteCount()) * kFrameBuffers);
}

SECURITY_ATTRIBUTES PermissiveAttributes(SECURITY_DESCRIPTOR* descriptor) {
    InitializeSecurityDescriptor(descriptor, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(descriptor, TRUE, nullptr, FALSE);
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = descriptor;
    attributes.bInheritHandle = FALSE;
    return attributes;
}

void InitializeHeader(FrameBusHeader* header, uint8_t* base) {
    std::memset(base, 0, MappingBytes());
    header->magic = kFrameBusMagic;
    header->version = kFrameBusVersion;
    header->width = kFrameWidth;
    header->height = kFrameHeight;
    header->stride = kFrameStride;
    header->fps = kFrameFps;
    header->bufferSize = FrameByteCount();
    header->published = 0;
    header->sequence = 0;
    header->heartbeatMs = 0;
    FrameBus::FillBlack(base + sizeof(FrameBusHeader), FrameByteCount());
    FrameBus::FillBlack(base + sizeof(FrameBusHeader) + FrameByteCount(), FrameByteCount());
}

}  // namespace

FrameBus::~FrameBus() {
    Close();
}

bool FrameBus::Create(const wchar_t* name) {
    Close();
    SECURITY_DESCRIPTOR descriptor{};
    SECURITY_ATTRIBUTES attributes = PermissiveAttributes(&descriptor);
    HANDLE mapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        &attributes,
        PAGE_READWRITE,
        0,
        MappingBytes(),
        name
    );
    if (mapping == nullptr) {
        return false;
    }
    const bool created = GetLastError() != ERROR_ALREADY_EXISTS;
    if (!Map(mapping, created)) {
        return false;
    }
    EnsureEvent(name);
    return true;
}

bool FrameBus::Open(const wchar_t* name) {
    Close();
    HANDLE mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (mapping == nullptr) {
        return false;
    }
    if (!Map(mapping, false)) {
        return false;
    }
    EnsureEvent(name);
    return true;
}

bool FrameBus::Map(void* handle, bool initialize) {
    mapping_ = handle;
    base_ = static_cast<uint8_t*>(MapViewOfFile(static_cast<HANDLE>(mapping_), FILE_MAP_ALL_ACCESS, 0, 0, MappingBytes()));
    if (base_ == nullptr) {
        Close();
        return false;
    }
    header_ = reinterpret_cast<FrameBusHeader*>(base_);
    if (initialize || header_->magic != kFrameBusMagic || header_->version != kFrameBusVersion || header_->bufferSize != FrameByteCount()) {
        InitializeHeader(header_, base_);
    }
    return true;
}

void FrameBus::EnsureEvent(const wchar_t* mappingName) {
    if (event_ != nullptr || mappingName == nullptr) {
        return;
    }
    wchar_t name[160] = {};
    wcscpy_s(name, mappingName);
    wcscat_s(name, L".Frame");
    SECURITY_DESCRIPTOR descriptor{};
    SECURITY_ATTRIBUTES attributes = PermissiveAttributes(&descriptor);
    event_ = CreateEventW(&attributes, FALSE, FALSE, name);
}

void FrameBus::Close() {
    header_ = nullptr;
    if (base_ != nullptr) {
        UnmapViewOfFile(base_);
        base_ = nullptr;
    }
    if (mapping_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(mapping_));
        mapping_ = nullptr;
    }
    if (event_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(event_));
        event_ = nullptr;
    }
}

uint8_t* FrameBus::Buffer(uint32_t index) const {
    return base_ + sizeof(FrameBusHeader) + static_cast<size_t>(FrameByteCount()) * (index % kFrameBuffers);
}

bool FrameBus::Publish(const uint8_t* nv12, uint32_t bytes, uint64_t captureMs) {
    if (header_ == nullptr || nv12 == nullptr || bytes != FrameByteCount()) {
        return false;
    }
    const uint32_t index = (header_->published + 1u) % kFrameBuffers;
    std::memcpy(Buffer(index), nv12, bytes);
    MemoryBarrier();
    header_->captureMs = captureMs;
    header_->published = index;
    InterlockedIncrement(reinterpret_cast<volatile LONG*>(&header_->sequence));
    header_->heartbeatMs = GetTickCount64();
    if (event_ != nullptr) {
        SetEvent(static_cast<HANDLE>(event_));
    }
    return true;
}

bool FrameBus::CopyLatest(uint8_t* nv12, uint32_t bytes) const {
    if (!Alive() || nv12 == nullptr || bytes != FrameByteCount()) {
        return false;
    }
    uint32_t sequence = header_->sequence;
    MemoryBarrier();
    std::memcpy(nv12, Buffer(header_->published), bytes);
    MemoryBarrier();
    if (header_->sequence != sequence) {
        std::memcpy(nv12, Buffer(header_->published), bytes);
    }
    return true;
}

uint32_t FrameBus::Sequence() const {
    if (header_ == nullptr) {
        return 0;
    }
    return header_->sequence;
}

uint64_t FrameBus::CaptureMs() const {
    if (header_ == nullptr) {
        return 0;
    }
    return header_->captureMs;
}

uint32_t FrameBus::Repeats() const {
    if (header_ == nullptr) {
        return 0;
    }
    return header_->repeats;
}

void FrameBus::NoteRepeat() {
    if (header_ == nullptr) {
        return;
    }
    InterlockedIncrement(reinterpret_cast<volatile LONG*>(&header_->repeats));
}

bool FrameBus::Alive() const {
    if (header_ == nullptr || header_->magic != kFrameBusMagic || header_->sequence == 0) {
        return false;
    }
    const uint64_t heartbeat = header_->heartbeatMs;
    const uint64_t now = GetTickCount64();
    return heartbeat != 0 && now >= heartbeat && (now - heartbeat) <= kFrameHeartbeatMs;
}

void FrameBus::FillBlack(uint8_t* nv12, uint32_t bytes) {
    if (nv12 == nullptr || bytes < FrameByteCount()) {
        return;
    }
    const uint32_t luma = kFrameStride * kFrameHeight;
    std::memset(nv12, 16, luma);
    std::memset(nv12 + luma, 128, luma / 2);
}

}  // namespace vcam
