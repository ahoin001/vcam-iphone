#include "MediaSource.h"

#include "camera/CameraIds.h"

#include <mfapi.h>
#include <mferror.h>

#include <cstring>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

namespace vcam {
namespace {

long g_objects = 0;

void Track(long delta) {
    InterlockedExchangeAdd(&g_objects, delta);
}

HRESULT EmptyVariant(PROPVARIANT* value) {
    PropVariantInit(value);
    value->vt = VT_EMPTY;
    return S_OK;
}

HRESULT CreateMediaType(IMFMediaType** type) {
    if (type == nullptr) {
        return E_POINTER;
    }
    IMFMediaType* media = nullptr;
    HRESULT hr = MFCreateMediaType(&media);
    if (FAILED(hr)) {
        return hr;
    }
    media->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    media->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(media, MF_MT_FRAME_SIZE, kFrameWidth, kFrameHeight);
    MFSetAttributeRatio(media, MF_MT_FRAME_RATE, kFrameFps, 1);
    MFSetAttributeRatio(media, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    MFSetAttributeRatio(media, MF_MT_FRAME_RATE_RANGE_MIN, kFrameFps, 1);
    MFSetAttributeRatio(media, MF_MT_FRAME_RATE_RANGE_MAX, kFrameFps, 1);
    media->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    media->SetUINT32(MF_MT_DEFAULT_STRIDE, kFrameStride);
    media->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    media->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE);
    media->SetUINT32(MF_MT_SAMPLE_SIZE, FrameByteCount());
    media->SetUINT32(MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_BT709);
    media->SetUINT32(MF_MT_YUV_MATRIX, MFVideoTransferMatrix_BT709);
    media->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_16_235);
    media->SetUINT32(MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_709);
    *type = media;
    return S_OK;
}

}  // namespace

long ObjectCount() {
    return g_objects;
}

MediaSource::MediaSource() {
    Track(1);
    MFCreateEventQueue(&queue_);
    stream_ = new MediaStream(this);
}

MediaSource::~MediaSource() {
    Shutdown();
    if (queue_ != nullptr) {
        queue_->Release();
        queue_ = nullptr;
    }
    Track(-1);
}

STDMETHODIMP MediaSource::QueryInterface(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IMFMediaEventGenerator || riid == IID_IMFMediaSource) {
        *object = static_cast<IMFMediaSource*>(this);
        AddRef();
        return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MediaSource::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&refCount_));
}

STDMETHODIMP_(ULONG) MediaSource::Release() {
    const ULONG count = static_cast<ULONG>(InterlockedDecrement(&refCount_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP MediaSource::BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) {
    return queue_ == nullptr ? MF_E_SHUTDOWN : queue_->BeginGetEvent(callback, state);
}

STDMETHODIMP MediaSource::EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) {
    return queue_ == nullptr ? MF_E_SHUTDOWN : queue_->EndGetEvent(result, event);
}

STDMETHODIMP MediaSource::GetEvent(DWORD flags, IMFMediaEvent** event) {
    return queue_ == nullptr ? MF_E_SHUTDOWN : queue_->GetEvent(flags, event);
}

STDMETHODIMP MediaSource::QueueEvent(MediaEventType type, REFGUID extendedType, HRESULT status, const PROPVARIANT* value) {
    return queue_ == nullptr ? MF_E_SHUTDOWN : queue_->QueueEventParamVar(type, extendedType, status, value);
}

STDMETHODIMP MediaSource::GetCharacteristics(DWORD* characteristics) {
    if (characteristics == nullptr) {
        return E_POINTER;
    }
    if (shutdown_) {
        return MF_E_SHUTDOWN;
    }
    *characteristics = MFMEDIASOURCE_IS_LIVE | MFMEDIASOURCE_DOES_NOT_USE_NETWORK;
    return S_OK;
}

HRESULT MediaSource::CreateDescriptor(IMFPresentationDescriptor** descriptor) {
    if (stream_ == nullptr || stream_->Descriptor() == nullptr) {
        return E_FAIL;
    }
    IMFStreamDescriptor* streams[] = {stream_->Descriptor()};
    IMFPresentationDescriptor* presentation = nullptr;
    HRESULT hr = MFCreatePresentationDescriptor(1, streams, &presentation);
    if (FAILED(hr)) {
        return hr;
    }
    presentation->SelectStream(0);
    presentation->SetString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, kCameraName);
    *descriptor = presentation;
    return S_OK;
}

STDMETHODIMP MediaSource::CreatePresentationDescriptor(IMFPresentationDescriptor** descriptor) {
    if (descriptor == nullptr) {
        return E_POINTER;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (shutdown_) {
        return MF_E_SHUTDOWN;
    }
    return CreateDescriptor(descriptor);
}

STDMETHODIMP MediaSource::Start(IMFPresentationDescriptor* descriptor, const GUID* timeFormat, const PROPVARIANT* startPosition) {
    if (timeFormat != nullptr && *timeFormat != GUID_NULL) {
        return MF_E_UNSUPPORTED_TIME_FORMAT;
    }
    MediaStream* stream = nullptr;
    bool firstStream = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            return MF_E_SHUTDOWN;
        }
        firstStream = !sentStream_;
        sentStream_ = true;
        if (descriptor != nullptr) {
            BOOL selected = FALSE;
            IMFStreamDescriptor* incoming = nullptr;
            if (SUCCEEDED(descriptor->GetStreamDescriptorByIndex(0, &selected, &incoming)) && incoming != nullptr) {
                if (!selected) {
                    descriptor->SelectStream(0);
                }
                incoming->Release();
            }
        }
        stream = stream_;
        if (stream != nullptr) {
            stream->AddRef();
        }
        started_ = true;
    }
    PROPVARIANT when;
    if (startPosition != nullptr) {
        PropVariantCopy(&when, startPosition);
    } else {
        EmptyVariant(&when);
    }
    if (stream != nullptr && queue_ != nullptr) {
        PROPVARIANT streamValue;
        PropVariantInit(&streamValue);
        streamValue.vt = VT_UNKNOWN;
        streamValue.punkVal = stream;
        stream->AddRef();
        queue_->QueueEventParamVar(firstStream ? MENewStream : MEUpdatedStream, GUID_NULL, S_OK, &streamValue);
        PropVariantClear(&streamValue);
        stream->Start(&when);
        stream->Release();
    }
    if (queue_ != nullptr) {
        queue_->QueueEventParamVar(MESourceStarted, GUID_NULL, S_OK, &when);
    }
    PropVariantClear(&when);
    return S_OK;
}

STDMETHODIMP MediaSource::Stop() {
    MediaStream* stream = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            return MF_E_SHUTDOWN;
        }
        started_ = false;
        stream = stream_;
        if (stream != nullptr) {
            stream->AddRef();
        }
    }
    if (stream != nullptr) {
        stream->Stop();
        stream->Release();
    }
    PROPVARIANT empty;
    EmptyVariant(&empty);
    if (queue_ != nullptr) {
        queue_->QueueEventParamVar(MESourceStopped, GUID_NULL, S_OK, &empty);
    }
    PropVariantClear(&empty);
    return S_OK;
}

STDMETHODIMP MediaSource::Pause() {
    return MF_E_INVALID_STATE_TRANSITION;
}

STDMETHODIMP MediaSource::Shutdown() {
    MediaStream* stream = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            return S_OK;
        }
        shutdown_ = true;
        started_ = false;
        stream = stream_;
        stream_ = nullptr;
    }
    if (stream != nullptr) {
        stream->Stop();
        stream->Release();
    }
    if (queue_ != nullptr) {
        queue_->Shutdown();
    }
    return S_OK;
}

MediaStream::MediaStream(MediaSource* source) : source_(source) {
    MFCreateEventQueue(&queue_);
    wake_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    IMFMediaType* type = nullptr;
    if (SUCCEEDED(CreateMediaType(&type)) && type != nullptr) {
        MFCreateStreamDescriptor(0, 1, &type, &descriptor_);
        type->Release();
    }
}

MediaStream::~MediaStream() {
    Stop();
    if (descriptor_ != nullptr) {
        descriptor_->Release();
        descriptor_ = nullptr;
    }
    if (queue_ != nullptr) {
        queue_->Release();
        queue_ = nullptr;
    }
    source_ = nullptr;
    if (wake_ != nullptr) {
        CloseHandle(wake_);
        wake_ = nullptr;
    }
    while (!requests_.empty()) {
        if (requests_.front() != nullptr) {
            requests_.front()->Release();
        }
        requests_.pop();
    }
}

STDMETHODIMP MediaStream::QueryInterface(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IMFMediaEventGenerator || riid == IID_IMFMediaStream) {
        *object = static_cast<IMFMediaStream*>(this);
        AddRef();
        return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MediaStream::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&refCount_));
}

STDMETHODIMP_(ULONG) MediaStream::Release() {
    const ULONG count = static_cast<ULONG>(InterlockedDecrement(&refCount_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP MediaStream::BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) {
    return queue_ == nullptr ? MF_E_SHUTDOWN : queue_->BeginGetEvent(callback, state);
}

STDMETHODIMP MediaStream::EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) {
    return queue_ == nullptr ? MF_E_SHUTDOWN : queue_->EndGetEvent(result, event);
}

STDMETHODIMP MediaStream::GetEvent(DWORD flags, IMFMediaEvent** event) {
    return queue_ == nullptr ? MF_E_SHUTDOWN : queue_->GetEvent(flags, event);
}

STDMETHODIMP MediaStream::QueueEvent(MediaEventType type, REFGUID extendedType, HRESULT status, const PROPVARIANT* value) {
    return queue_ == nullptr ? MF_E_SHUTDOWN : queue_->QueueEventParamVar(type, extendedType, status, value);
}

STDMETHODIMP MediaStream::GetMediaSource(IMFMediaSource** source) {
    if (source == nullptr) {
        return E_POINTER;
    }
    if (source_ == nullptr) {
        return MF_E_SHUTDOWN;
    }
    *source = source_;
    source_->AddRef();
    return S_OK;
}

STDMETHODIMP MediaStream::GetStreamDescriptor(IMFStreamDescriptor** descriptor) {
    if (descriptor == nullptr) {
        return E_POINTER;
    }
    if (descriptor_ == nullptr) {
        return E_FAIL;
    }
    *descriptor = descriptor_;
    descriptor_->AddRef();
    return S_OK;
}

STDMETHODIMP MediaStream::RequestSample(IUnknown* token) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_ || !started_) {
        return MF_E_MEDIA_SOURCE_WRONGSTATE;
    }
    if (token != nullptr) {
        token->AddRef();
    }
    if (requests_.size() >= 4) {
        if (requests_.front() != nullptr) {
            requests_.front()->Release();
        }
        requests_.pop();
    }
    requests_.push(token);
    if (wake_ != nullptr) {
        SetEvent(wake_);
    }
    return S_OK;
}

HRESULT MediaStream::Start(const PROPVARIANT* startPosition) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = false;
        started_ = true;
        frameIndex_ = 0;
        if (!thread_.joinable()) {
            thread_ = std::thread([this]() { Run(); });
        }
    }
    PROPVARIANT when;
    if (startPosition != nullptr) {
        PropVariantCopy(&when, startPosition);
    } else {
        EmptyVariant(&when);
    }
    if (queue_ != nullptr) {
        queue_->QueueEventParamVar(MEStreamStarted, GUID_NULL, S_OK, &when);
    }
    PropVariantClear(&when);
    if (wake_ != nullptr) {
        SetEvent(wake_);
    }
    return S_OK;
}

HRESULT MediaStream::Stop() {
    std::thread worker;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_ && !thread_.joinable()) {
            stop_ = true;
        }
        started_ = false;
        stop_ = true;
        worker = std::move(thread_);
        while (!requests_.empty()) {
            if (requests_.front() != nullptr) {
                requests_.front()->Release();
            }
            requests_.pop();
        }
    }
    if (wake_ != nullptr) {
        SetEvent(wake_);
    }
    if (worker.joinable()) {
        worker.join();
    }
    bus_.Close();
    PROPVARIANT empty;
    EmptyVariant(&empty);
    if (queue_ != nullptr) {
        queue_->QueueEventParamVar(MEStreamStopped, GUID_NULL, S_OK, &empty);
    }
    PropVariantClear(&empty);
    return S_OK;
}

void MediaStream::Run() {
    while (true) {
        WaitForSingleObject(wake_, 33);
        IUnknown* token = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stop_) {
                return;
            }
            if (!started_ || requests_.empty()) {
                continue;
            }
            token = requests_.front();
            requests_.pop();
        }
        Deliver(token);
        if (token != nullptr) {
            token->Release();
        }
    }
}

bool MediaStream::AttachBus() {
    if (bus_.IsOpen()) {
        return true;
    }
    if (bus_.Open(kGlobalFrameBusName) || bus_.Create(kGlobalFrameBusName)) {
        return true;
    }
    return bus_.Open(kLocalFrameBusName) || bus_.Create(kLocalFrameBusName);
}

HRESULT MediaStream::Deliver(IUnknown* token) {
    if (queue_ == nullptr) {
        return MF_E_SHUTDOWN;
    }
    IMFMediaBuffer* buffer = nullptr;
    HRESULT hr = MFCreateMemoryBuffer(FrameByteCount(), &buffer);
    if (FAILED(hr) || buffer == nullptr) {
        return hr;
    }
    BYTE* bytes = nullptr;
    DWORD maxLength = 0;
    hr = buffer->Lock(&bytes, &maxLength, nullptr);
    if (FAILED(hr) || bytes == nullptr) {
        buffer->Release();
        return hr;
    }
    if (maxLength < FrameByteCount()) {
        buffer->Unlock();
        buffer->Release();
        return E_FAIL;
    }
    if (!AttachBus() || !bus_.CopyLatest(bytes, FrameByteCount())) {
        FrameBus::FillBlack(bytes, FrameByteCount());
    }
    buffer->Unlock();
    buffer->SetCurrentLength(FrameByteCount());

    IMFSample* sample = nullptr;
    hr = MFCreateSample(&sample);
    if (FAILED(hr) || sample == nullptr) {
        buffer->Release();
        return hr;
    }
    sample->AddBuffer(buffer);
    buffer->Release();
    const LONGLONG time = frameIndex_ * 10000000 / kFrameFps;
    const LONGLONG duration = 10000000 / kFrameFps;
    frameIndex_ += 1;
    sample->SetSampleTime(time);
    sample->SetSampleDuration(duration);
    if (token != nullptr) {
        sample->SetUnknown(MFSampleExtension_Token, token);
    }
    PROPVARIANT value;
    PropVariantInit(&value);
    value.vt = VT_UNKNOWN;
    value.punkVal = sample;
    sample->AddRef();
    hr = queue_->QueueEventParamVar(MEMediaSample, GUID_NULL, S_OK, &value);
    PropVariantClear(&value);
    sample->Release();
    return hr;
}

}  // namespace vcam
