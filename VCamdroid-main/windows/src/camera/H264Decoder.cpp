#include "H264Decoder.h"

#include "FrameBus.h"

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mftransform.h>

#include <cstring>
#include <vector>

namespace vcam {
namespace {

constexpr uint64_t kNoPts = ~uint64_t{0};

struct Nal {
    const uint8_t* data = nullptr;
    size_t size = 0;
    uint8_t type = 0;
};

void SplitAnnexB(const uint8_t* data, size_t size, std::vector<Nal>* out);

size_t FindStart(const uint8_t* data, size_t size, size_t at, size_t* codeBytes) {
    for (size_t index = at; index + 3 < size; ++index) {
        if (index + 4 <= size && data[index] == 0 && data[index + 1] == 0 && data[index + 2] == 0 && data[index + 3] == 1) {
            *codeBytes = 4;
            return index;
        }
        if (data[index] == 0 && data[index + 1] == 0 && data[index + 2] == 1) {
            *codeBytes = 3;
            return index;
        }
    }
    return size;
}

void AppendLength(std::vector<uint8_t>* out, const uint8_t* data, size_t size) {
    out->push_back(static_cast<uint8_t>((size >> 24) & 0xff));
    out->push_back(static_cast<uint8_t>((size >> 16) & 0xff));
    out->push_back(static_cast<uint8_t>((size >> 8) & 0xff));
    out->push_back(static_cast<uint8_t>(size & 0xff));
    out->insert(out->end(), data, data + size);
}

std::vector<uint8_t> BuildAvcc(const std::vector<uint8_t>& sps, const std::vector<uint8_t>& pps) {
    std::vector<uint8_t> avcc;
    if (sps.size() < 4 || pps.empty()) {
        return avcc;
    }
    avcc.push_back(1);
    avcc.push_back(sps[1]);
    avcc.push_back(sps[2]);
    avcc.push_back(sps[3]);
    avcc.push_back(0xff);
    avcc.push_back(0xe1);
    avcc.push_back(static_cast<uint8_t>((sps.size() >> 8) & 0xff));
    avcc.push_back(static_cast<uint8_t>(sps.size() & 0xff));
    avcc.insert(avcc.end(), sps.begin(), sps.end());
    avcc.push_back(1);
    avcc.push_back(static_cast<uint8_t>((pps.size() >> 8) & 0xff));
    avcc.push_back(static_cast<uint8_t>(pps.size() & 0xff));
    avcc.insert(avcc.end(), pps.begin(), pps.end());
    return avcc;
}

void ScaleNv12(const uint8_t* source, int sourceWidth, int sourceHeight, int stride, uint8_t* destination) {
    const int destWidth = static_cast<int>(kFrameWidth);
    const int destHeight = static_cast<int>(kFrameHeight);
    if (sourceWidth == destWidth && sourceHeight == destHeight && stride == destWidth) {
        std::memcpy(destination, source, FrameByteCount());
        return;
    }
    for (int y = 0; y < destHeight; ++y) {
        const int sourceY = y * sourceHeight / destHeight;
        const uint8_t* row = source + static_cast<size_t>(sourceY) * static_cast<size_t>(stride);
        uint8_t* out = destination + static_cast<size_t>(y) * kFrameStride;
        for (int x = 0; x < destWidth; ++x) {
            out[x] = row[x * sourceWidth / destWidth];
        }
    }
    const uint8_t* sourceChroma = source + static_cast<size_t>(stride) * static_cast<size_t>(sourceHeight);
    uint8_t* destChroma = destination + kFrameStride * kFrameHeight;
    const int chromaHeight = destHeight / 2;
    const int sourceChromaHeight = sourceHeight / 2;
    const int sourceChromaWidth = sourceWidth / 2;
    for (int y = 0; y < chromaHeight; ++y) {
        const int sourceY = sourceChromaHeight == 0 ? 0 : y * sourceChromaHeight / chromaHeight;
        const uint8_t* row = sourceChroma + static_cast<size_t>(sourceY) * static_cast<size_t>(stride);
        uint8_t* out = destChroma + static_cast<size_t>(y) * kFrameStride;
        for (int x = 0; x < destWidth; x += 2) {
            const int sourceX = sourceChromaWidth == 0 ? 0 : (x / 2) * sourceChromaWidth / (destWidth / 2) * 2;
            out[x] = row[sourceX];
            out[x + 1] = row[sourceX + 1];
        }
    }
}

void SplitAnnexB(const uint8_t* data, size_t size, std::vector<Nal>* out) {
    size_t codeBytes = 0;
    size_t start = FindStart(data, size, 0, &codeBytes);
    while (start < size) {
        const size_t nal = start + codeBytes;
        size_t nextCode = 0;
        const size_t next = FindStart(data, size, nal, &nextCode);
        if (nal < size && next > nal) {
            Nal view;
            view.data = data + nal;
            view.size = next - nal;
            while (view.size > 0 && view.data[view.size - 1] == 0) {
                view.size -= 1;
            }
            if (view.size > 0) {
                view.type = view.data[0] & 0x1f;
                out->push_back(view);
            }
        }
        start = next;
        codeBytes = nextCode;
    }
}

}  // namespace

bool CopyOutput(::IMFSample* sample, ::IMFTransform* transform, std::vector<uint8_t>& nv12);

H264Decoder::H264Decoder() = default;

H264Decoder::~H264Decoder() {
    if (decoder_ != nullptr) {
        static_cast<IMFTransform*>(decoder_)->Release();
        decoder_ = nullptr;
    }
}

bool H264Decoder::Decode(uint64_t pts, const uint8_t* annexB, size_t size, std::vector<uint8_t>& nv12) {
    if (annexB == nullptr || size == 0) {
        return false;
    }
    std::vector<Nal> nals;
    SplitAnnexB(annexB, size, &nals);
    bool parameterChange = false;
    for (const Nal& nal : nals) {
        if (nal.type == 7 && (sps_.size() != nal.size || std::memcmp(sps_.data(), nal.data, nal.size) != 0)) {
            sps_.assign(nal.data, nal.data + nal.size);
            parameterChange = true;
        } else if (nal.type == 8 && (pps_.size() != nal.size || std::memcmp(pps_.data(), nal.data, nal.size) != 0)) {
            pps_.assign(nal.data, nal.data + nal.size);
            parameterChange = true;
        }
    }
    if (parameterChange) {
        configured_ = false;
    }
    if (pts == kNoPts) {
        return false;
    }
    std::vector<uint8_t> sampleBytes;
    for (const Nal& nal : nals) {
        if (nal.type == 1 || nal.type == 5 || nal.type == 6 || nal.type == 7 || nal.type == 8) {
            AppendLength(&sampleBytes, nal.data, nal.size);
        }
    }
    if (sampleBytes.empty() || !EnsureDecoder() || !Configure()) {
        return false;
    }

    IMFSample* sample = nullptr;
    if (FAILED(MFCreateSample(&sample)) || sample == nullptr) {
        return false;
    }
    IMFMediaBuffer* buffer = nullptr;
    if (FAILED(MFCreateMemoryBuffer(static_cast<DWORD>(sampleBytes.size()), &buffer)) || buffer == nullptr) {
        sample->Release();
        return false;
    }
    BYTE* destination = nullptr;
    if (FAILED(buffer->Lock(&destination, nullptr, nullptr)) || destination == nullptr) {
        buffer->Release();
        sample->Release();
        return false;
    }
    std::memcpy(destination, sampleBytes.data(), sampleBytes.size());
    buffer->Unlock();
    buffer->SetCurrentLength(static_cast<DWORD>(sampleBytes.size()));
    sample->AddBuffer(buffer);
    buffer->Release();
    if (pts != kNoPts) {
        sample->SetSampleTime(static_cast<LONGLONG>(pts) * 10000);
    }
    sample->SetSampleDuration(10000000 / kFrameFps);

    auto* transform = static_cast<IMFTransform*>(decoder_);
    HRESULT hr = transform->ProcessInput(0, sample, 0);
    if (hr == MF_E_NOTACCEPTING) {
        Drain(nv12);
        hr = transform->ProcessInput(0, sample, 0);
    }
    sample->Release();
    if (FAILED(hr)) {
        return false;
    }
    return Drain(nv12);
}

bool H264Decoder::EnsureDecoder() {
    if (decoder_ != nullptr) {
        return true;
    }
    MFT_REGISTER_TYPE_INFO input{MFMediaType_Video, MFVideoFormat_H264};
    IMFActivate** activates = nullptr;
    UINT32 count = 0;
    const HRESULT listed = MFTEnumEx(
        MFT_CATEGORY_VIDEO_DECODER,
        MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER,
        &input,
        nullptr,
        &activates,
        &count
    );
    if (FAILED(listed) || activates == nullptr) {
        return false;
    }
    for (UINT32 index = 0; index < count; ++index) {
        IMFTransform* transform = nullptr;
        if (SUCCEEDED(activates[index]->ActivateObject(IID_PPV_ARGS(&transform))) && transform != nullptr) {
            decoder_ = transform;
            break;
        }
    }
    for (UINT32 index = 0; index < count; ++index) {
        activates[index]->Release();
    }
    CoTaskMemFree(activates);
    return decoder_ != nullptr;
}

bool H264Decoder::Configure() {
    if (configured_) {
        return true;
    }
    avcc_ = BuildAvcc(sps_, pps_);
    if (avcc_.empty() || decoder_ == nullptr) {
        return false;
    }
    auto* transform = static_cast<IMFTransform*>(decoder_);
    IMFMediaType* input = nullptr;
    if (FAILED(MFCreateMediaType(&input)) || input == nullptr) {
        return false;
    }
    input->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    input->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    MFSetAttributeSize(input, MF_MT_FRAME_SIZE, kFrameWidth, kFrameHeight);
    MFSetAttributeRatio(input, MF_MT_FRAME_RATE, kFrameFps, 1);
    MFSetAttributeRatio(input, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    input->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    input->SetBlob(MF_MT_MPEG_SEQUENCE_HEADER, avcc_.data(), static_cast<UINT32>(avcc_.size()));
    const HRESULT hr = transform->SetInputType(0, input, 0);
    input->Release();
    if (FAILED(hr)) {
        return false;
    }
    configured_ = ChooseOutput();
    return configured_;
}

bool H264Decoder::ChooseOutput() {
    auto* transform = static_cast<IMFTransform*>(decoder_);
    for (DWORD index = 0;; ++index) {
        IMFMediaType* type = nullptr;
        const HRESULT hr = transform->GetOutputAvailableType(0, index, &type);
        if (FAILED(hr) || type == nullptr) {
            break;
        }
        GUID subtype = GUID_NULL;
        type->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (subtype == MFVideoFormat_NV12) {
            const HRESULT set = transform->SetOutputType(0, type, 0);
            type->Release();
            if (FAILED(set)) {
                return false;
            }
            MFT_OUTPUT_STREAM_INFO info{};
            transform->GetOutputStreamInfo(0, &info);
            providesSamples_ = (info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0;
            outputBytes_ = info.cbSize == 0 ? FrameByteCount() : info.cbSize;
            return true;
        }
        type->Release();
    }
    return false;
}

bool H264Decoder::Drain(std::vector<uint8_t>& nv12) {
    auto* transform = static_cast<IMFTransform*>(decoder_);
    bool produced = false;
    for (int attempt = 0; attempt < 4; ++attempt) {
        MFT_OUTPUT_DATA_BUFFER output{};
        IMFSample* sample = nullptr;
        IMFMediaBuffer* buffer = nullptr;
        if (!providesSamples_) {
            if (FAILED(MFCreateSample(&sample)) || sample == nullptr) {
                return produced;
            }
            if (FAILED(MFCreateMemoryBuffer(outputBytes_ == 0 ? FrameByteCount() : outputBytes_, &buffer)) || buffer == nullptr) {
                sample->Release();
                return produced;
            }
            sample->AddBuffer(buffer);
            buffer->Release();
            output.pSample = sample;
        }
        DWORD status = 0;
        const HRESULT hr = transform->ProcessOutput(0, 1, &output, &status);
        if (output.pEvents != nullptr) {
            output.pEvents->Release();
        }
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            if (sample != nullptr) {
                sample->Release();
            }
            return produced;
        }
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
            if (sample != nullptr) {
                sample->Release();
            }
            if (!ChooseOutput()) {
                return produced;
            }
            continue;
        }
        if (FAILED(hr) || output.pSample == nullptr) {
            if (sample != nullptr) {
                sample->Release();
            }
            return produced;
        }
        produced = CopyOutput(output.pSample, transform, nv12) || produced;
        output.pSample->Release();
    }
    return produced;
}

bool CopyOutput(::IMFSample* sample, IMFTransform* transform, std::vector<uint8_t>& nv12) {
    if (sample == nullptr) {
        return false;
    }
    IMFMediaBuffer* buffer = nullptr;
    if (FAILED(sample->ConvertToContiguousBuffer(&buffer)) || buffer == nullptr) {
        return false;
    }
    UINT32 width = kFrameWidth;
    UINT32 height = kFrameHeight;
    IMFMediaType* type = nullptr;
    if (SUCCEEDED(transform->GetOutputCurrentType(0, &type)) && type != nullptr) {
        MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height);
        type->Release();
    }
    BYTE* scan = nullptr;
    LONG stride = static_cast<LONG>(width);
    DWORD length = 0;
    IMF2DBuffer* twoD = nullptr;
    bool locked2D = false;
    if (SUCCEEDED(buffer->QueryInterface(IID_PPV_ARGS(&twoD))) && twoD != nullptr) {
        if (SUCCEEDED(twoD->Lock2D(&scan, &stride))) {
            locked2D = true;
        }
    }
    if (!locked2D) {
        if (FAILED(buffer->Lock(&scan, nullptr, &length)) || scan == nullptr) {
            if (twoD != nullptr) {
                twoD->Release();
            }
            buffer->Release();
            return false;
        }
        stride = static_cast<LONG>(width);
    }
    nv12.resize(FrameByteCount());
    const int step = stride < 0 ? -stride : stride;
    const uint8_t* source = scan;
    if (stride < 0) {
        source = scan + static_cast<size_t>(step) * static_cast<size_t>(height > 0 ? height - 1 : 0);
    }
    if (width == 0 || height == 0) {
        width = kFrameWidth;
        height = kFrameHeight;
    }
    ScaleNv12(source, static_cast<int>(width), static_cast<int>(height), step, nv12.data());
    if (locked2D) {
        twoD->Unlock2D();
    } else {
        buffer->Unlock();
    }
    if (twoD != nullptr) {
        twoD->Release();
    }
    buffer->Release();
    return true;
}

}  // namespace vcam
