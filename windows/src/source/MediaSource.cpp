#include "MediaSource.h"

#include "camera/CameraIds.h"

#include <mfapi.h>
#include <mferror.h>

#include <cstring>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

namespace vcam {
namespace {

// {FB6C4281-0353-11D1-905F-0000C0CC16BA}
constexpr GUID kPinVideoCapture = {0xfb6c4281, 0x0353, 0x11d1, {0x90, 0x5f, 0x00, 0x00, 0xc0, 0xcc, 0x16, 0xba}};
// MF_DEVICESTREAM_FRAMESERVER_SHARED {1CB378E9-B279-41D4-AF97-34A243E68320}
constexpr GUID kFrameServerShared = {0x1cb378e9, 0xb279, 0x41d4, {0xaf, 0x97, 0x34, 0xa2, 0x43, 0xe6, 0x83, 0x20}};

long g_objects = 0;

HRESULT EmptyVariant(PROPVARIANT* value) {
    PropVariantInit(value);
    value->vt = VT_EMPTY;
    return S_OK;
}

HRESULT NoKsSupport(ULONG* bytesReturned) {
    if (bytesReturned != nullptr) {
        *bytesReturned = 0;
    }
    return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
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

void TrackObject(long delta) {
    InterlockedExchangeAdd(&g_objects, delta);
}

MediaSource::MediaSource() {
    TrackObject(1);
    MFCreateEventQueue(&queue_);
    MFCreateAttributes(&attributes_, 4);
    stream_ = new MediaStream(this);
}

MediaSource::~MediaSource() {
    Shutdown();
    if (queue_ != nullptr) {
        queue_->Release();
        queue_ = nullptr;
    }
    if (attributes_ != nullptr) {
        attributes_->Release();
        attributes_ = nullptr;
    }
    TrackObject(-1);
}

HRESULT MediaSource::Initialize(IMFAttributes* activateAttributes) {
    if (attributes_ == nullptr) {
        return E_OUTOFMEMORY;
    }
    if (activateAttributes != nullptr) {
        const HRESULT hr = activateAttributes->CopyAllItems(attributes_);
        if (FAILED(hr)) {
            return hr;
        }
    }
    return S_OK;
}

STDMETHODIMP MediaSource::QueryInterface(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IMFMediaEventGenerator || riid == IID_IMFMediaSource ||
        riid == __uuidof(IMFMediaSourceEx)) {
        *object = static_cast<IMFMediaSourceEx*>(this);
    } else if (riid == __uuidof(IMFGetService)) {
        *object = static_cast<IMFGetService*>(this);
    } else if (riid == __uuidof(KsControl)) {
        *object = static_cast<KsControl*>(this);
    } else {
        *object = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
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
    if (startPosition != nullptr && startPosition->vt != VT_EMPTY) {
        PropVariantCopy(&when, startPosition);
    } else {
        PropVariantInit(&when);
        when.vt = VT_I8;
        when.hVal.QuadPart = MFGetSystemTime();
    }
    if (stream != nullptr && queue_ != nullptr) {
        queue_->QueueEventParamUnk(
            firstStream ? MENewStream : MEUpdatedStream,
            GUID_NULL,
            S_OK,
            static_cast<IMFMediaStream2*>(stream)
        );
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
        stream->Shutdown();
        stream->Release();
    }
    if (queue_ != nullptr) {
        queue_->Shutdown();
    }
    return S_OK;
}

STDMETHODIMP MediaSource::GetSourceAttributes(IMFAttributes** attributes) {
    if (attributes == nullptr) {
        return E_POINTER;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (shutdown_) {
        return MF_E_SHUTDOWN;
    }
    if (attributes_ == nullptr) {
        return E_FAIL;
    }
    *attributes = attributes_;
    attributes_->AddRef();
    return S_OK;
}

STDMETHODIMP MediaSource::GetStreamAttributes(DWORD streamId, IMFAttributes** attributes) {
    if (attributes == nullptr) {
        return E_POINTER;
    }
    *attributes = nullptr;
    std::lock_guard<std::mutex> lock(mutex_);
    if (shutdown_) {
        return MF_E_SHUTDOWN;
    }
    if (streamId != 0) {
        return MF_E_INVALIDSTREAMNUMBER;
    }
    if (stream_ == nullptr || stream_->Descriptor() == nullptr) {
        return E_FAIL;
    }
    return stream_->Descriptor()->QueryInterface(IID_IMFAttributes, reinterpret_cast<void**>(attributes));
}

STDMETHODIMP MediaSource::SetD3DManager(IUnknown*) {
    return shutdown_ ? MF_E_SHUTDOWN : S_OK;
}

STDMETHODIMP MediaSource::GetService(REFGUID, REFIID, LPVOID* object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    *object = nullptr;
    return MF_E_UNSUPPORTED_SERVICE;
}

STDMETHODIMP MediaSource::KsProperty(void*, ULONG, void*, ULONG, ULONG* bytesReturned) {
    return NoKsSupport(bytesReturned);
}

STDMETHODIMP MediaSource::KsMethod(void*, ULONG, void*, ULONG, ULONG* bytesReturned) {
    return NoKsSupport(bytesReturned);
}

STDMETHODIMP MediaSource::KsEvent(void*, ULONG, void*, ULONG, ULONG* bytesReturned) {
    return NoKsSupport(bytesReturned);
}

MediaStream::MediaStream(MediaSource* source) : source_(source) {
    TrackObject(1);
    MFCreateEventQueue(&queue_);
    wake_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    IMFMediaType* type = nullptr;
    if (SUCCEEDED(CreateMediaType(&type)) && type != nullptr) {
        if (SUCCEEDED(MFCreateStreamDescriptor(0, 1, &type, &descriptor_)) && descriptor_ != nullptr) {
            IMFMediaTypeHandler* handler = nullptr;
            if (SUCCEEDED(descriptor_->GetMediaTypeHandler(&handler)) && handler != nullptr) {
                handler->SetCurrentMediaType(type);
                handler->Release();
            }
            descriptor_->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, kPinVideoCapture);
            descriptor_->SetUINT32(MF_DEVICESTREAM_STREAM_ID, 0);
            descriptor_->SetUINT32(kFrameServerShared, 1);
        }
        type->Release();
    }
}

MediaStream::~MediaStream() {
    Shutdown();
    if (descriptor_ != nullptr) {
        descriptor_->Release();
        descriptor_ = nullptr;
    }
    if (queue_ != nullptr) {
        queue_->Release();
        queue_ = nullptr;
    }
    if (wake_ != nullptr) {
        CloseHandle(wake_);
        wake_ = nullptr;
    }
    TrackObject(-1);
}

STDMETHODIMP MediaStream::QueryInterface(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IMFMediaEventGenerator || riid == IID_IMFMediaStream ||
        riid == __uuidof(IMFMediaStream2)) {
        *object = static_cast<IMFMediaStream2*>(this);
    } else if (riid == __uuidof(KsControl)) {
        *object = static_cast<KsControl*>(this);
    } else {
        *object = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
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
    std::lock_guard<std::mutex> lock(mutex_);
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
    if (stop_ || state_ != MF_STREAM_STATE_RUNNING) {
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

STDMETHODIMP MediaStream::SetStreamState(MF_STREAM_STATE state) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (source_ == nullptr) {
            return MF_E_SHUTDOWN;
        }
        if (state == state_) {
            return S_OK;
        }
    }
    switch (state) {
    case MF_STREAM_STATE_RUNNING:
        return Start(nullptr);
    case MF_STREAM_STATE_PAUSED: {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = MF_STREAM_STATE_PAUSED;
        ClearRequests();
        return S_OK;
    }
    case MF_STREAM_STATE_STOPPED:
        return Stop();
    default:
        return MF_E_INVALID_STATE_TRANSITION;
    }
}

STDMETHODIMP MediaStream::GetStreamState(MF_STREAM_STATE* state) {
    if (state == nullptr) {
        return E_POINTER;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    *state = state_;
    return S_OK;
}

STDMETHODIMP MediaStream::KsProperty(void*, ULONG, void*, ULONG, ULONG* bytesReturned) {
    return NoKsSupport(bytesReturned);
}

STDMETHODIMP MediaStream::KsMethod(void*, ULONG, void*, ULONG, ULONG* bytesReturned) {
    return NoKsSupport(bytesReturned);
}

STDMETHODIMP MediaStream::KsEvent(void*, ULONG, void*, ULONG, ULONG* bytesReturned) {
    return NoKsSupport(bytesReturned);
}

void MediaStream::ClearRequests() {
    while (!requests_.empty()) {
        if (requests_.front() != nullptr) {
            requests_.front()->Release();
        }
        requests_.pop();
    }
}

HRESULT MediaStream::Start(const PROPVARIANT* startPosition) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (source_ == nullptr) {
            return MF_E_SHUTDOWN;
        }
        stop_ = false;
        state_ = MF_STREAM_STATE_RUNNING;
        if (!thread_.joinable()) {
            thread_ = std::thread([this]() { Run(); });
        }
    }
    PROPVARIANT when;
    if (startPosition != nullptr) {
        PropVariantCopy(&when, startPosition);
    } else {
        PropVariantInit(&when);
        when.vt = VT_I8;
        when.hVal.QuadPart = MFGetSystemTime();
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
        state_ = MF_STREAM_STATE_STOPPED;
        stop_ = true;
        worker = std::move(thread_);
        ClearRequests();
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

void MediaStream::Shutdown() {
    bool running = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running = thread_.joinable() || state_ != MF_STREAM_STATE_STOPPED;
    }
    if (running) {
        Stop();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        source_ = nullptr;
    }
    if (queue_ != nullptr) {
        queue_->Shutdown();
    }
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
            if (state_ != MF_STREAM_STATE_RUNNING || requests_.empty()) {
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
    sample->SetSampleTime(MFGetSystemTime());
    sample->SetSampleDuration(10000000 / kFrameFps);
    if (token != nullptr) {
        sample->SetUnknown(MFSampleExtension_Token, token);
    }
    hr = queue_->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, sample);
    sample->Release();
    return hr;
}

}  // namespace vcam
