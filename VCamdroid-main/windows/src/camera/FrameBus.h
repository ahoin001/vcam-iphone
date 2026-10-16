#pragma once

#include <cstdint>
#include <wchar.h>

// One NV12 picture shared by VCamPanel (the writer) and VCamSource (the reader).
// The media source runs in the Windows camera frame server, which is a different
// process and often a different session from the companion. Either side may create
// the mapping. Global\ is the cross-session name. Local\ covers a frame server
// that loads the source in the user session. The mapping uses a null DACL so
// those two processes can open the same picture.

namespace vcam {

constexpr wchar_t kGlobalFrameBusName[] = L"Global\\VCamdroid.FrameBus";
constexpr wchar_t kLocalFrameBusName[] = L"Local\\VCamdroid.FrameBus";
constexpr uint32_t kFrameBusMagic = 0x5643414Du;
constexpr uint32_t kFrameBusVersion = 1;
constexpr uint32_t kFrameWidth = 1920;
constexpr uint32_t kFrameHeight = 1080;
constexpr uint32_t kFrameFps = 30;
constexpr uint32_t kFrameStride = 1920;
constexpr uint32_t kFrameBuffers = 2;
constexpr uint32_t kFrameHeartbeatMs = 500;

constexpr uint32_t FrameByteCount() {
    return kFrameStride * kFrameHeight * 3 / 2;
}

struct FrameBusHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t fps;
    uint32_t bufferSize;
    uint32_t reserved;
    volatile uint32_t published;
    volatile uint32_t sequence;
    volatile uint64_t heartbeatMs;
};

class FrameBus {
public:
    FrameBus() = default;
    FrameBus(const FrameBus&) = delete;
    FrameBus& operator=(const FrameBus&) = delete;
    ~FrameBus();

    bool Create(const wchar_t* name);
    bool Open(const wchar_t* name);
    void Close();
    bool IsOpen() const { return header_ != nullptr; }

    bool Publish(const uint8_t* nv12, uint32_t bytes);
    bool CopyLatest(uint8_t* nv12, uint32_t bytes) const;
    bool Alive() const;

    static void FillBlack(uint8_t* nv12, uint32_t bytes);

private:
    bool Map(void* handle, bool initialize);
    uint8_t* Buffer(uint32_t index) const;

    void* mapping_ = nullptr;
    FrameBusHeader* header_ = nullptr;
    uint8_t* base_ = nullptr;
};

}  // namespace vcam
