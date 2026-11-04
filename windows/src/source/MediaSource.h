#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mfidl.h>

#include "camera/FrameBus.h"

#include <mutex>
#include <queue>
#include <thread>

namespace vcam {

long ObjectCount();
void TrackObject(long delta);

// The Windows camera frame server routes camera controls through IKsControl.
// Declared here so the source does not pull in the DirectShow-era ksproxy.h.
struct __declspec(uuid("28F54685-06FD-11D2-B27A-00A0C9223196")) KsControl : public IUnknown {
    STDMETHOD(KsProperty)(void* property, ULONG propertyLength, void* data, ULONG dataLength, ULONG* bytesReturned) = 0;
    STDMETHOD(KsMethod)(void* method, ULONG methodLength, void* data, ULONG dataLength, ULONG* bytesReturned) = 0;
    STDMETHOD(KsEvent)(void* event, ULONG eventLength, void* data, ULONG dataLength, ULONG* bytesReturned) = 0;
};

class MediaStream;

class MediaSource : public IMFMediaSourceEx, public IMFGetService, public KsControl {
public:
    MediaSource();
    virtual ~MediaSource();

    // Copies the attributes the frame server set on the activator.
    HRESULT Initialize(IMFAttributes* activateAttributes);

    STDMETHODIMP QueryInterface(REFIID riid, void** object) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) override;
    STDMETHODIMP EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) override;
    STDMETHODIMP GetEvent(DWORD flags, IMFMediaEvent** event) override;
    STDMETHODIMP QueueEvent(MediaEventType type, REFGUID extendedType, HRESULT status, const PROPVARIANT* value) override;

    STDMETHODIMP GetCharacteristics(DWORD* characteristics) override;
    STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor** descriptor) override;
    STDMETHODIMP Start(IMFPresentationDescriptor* descriptor, const GUID* timeFormat, const PROPVARIANT* startPosition) override;
    STDMETHODIMP Stop() override;
    STDMETHODIMP Pause() override;
    STDMETHODIMP Shutdown() override;

    STDMETHODIMP GetSourceAttributes(IMFAttributes** attributes) override;
    STDMETHODIMP GetStreamAttributes(DWORD streamId, IMFAttributes** attributes) override;
    STDMETHODIMP SetD3DManager(IUnknown* manager) override;

    STDMETHODIMP GetService(REFGUID service, REFIID riid, LPVOID* object) override;

    STDMETHODIMP KsProperty(void* property, ULONG propertyLength, void* data, ULONG dataLength, ULONG* bytesReturned) override;
    STDMETHODIMP KsMethod(void* method, ULONG methodLength, void* data, ULONG dataLength, ULONG* bytesReturned) override;
    STDMETHODIMP KsEvent(void* event, ULONG eventLength, void* data, ULONG dataLength, ULONG* bytesReturned) override;

private:
    HRESULT CreateDescriptor(IMFPresentationDescriptor** descriptor);

    long refCount_ = 1;
    std::mutex mutex_;
    IMFMediaEventQueue* queue_ = nullptr;
    IMFAttributes* attributes_ = nullptr;
    MediaStream* stream_ = nullptr;
    bool started_ = false;
    bool shutdown_ = false;
    bool sentStream_ = false;
};

class MediaStream : public IMFMediaStream2, public KsControl {
public:
    explicit MediaStream(MediaSource* source);
    virtual ~MediaStream();

    STDMETHODIMP QueryInterface(REFIID riid, void** object) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) override;
    STDMETHODIMP EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) override;
    STDMETHODIMP GetEvent(DWORD flags, IMFMediaEvent** event) override;
    STDMETHODIMP QueueEvent(MediaEventType type, REFGUID extendedType, HRESULT status, const PROPVARIANT* value) override;

    STDMETHODIMP GetMediaSource(IMFMediaSource** source) override;
    STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor** descriptor) override;
    STDMETHODIMP RequestSample(IUnknown* token) override;

    STDMETHODIMP SetStreamState(MF_STREAM_STATE state) override;
    STDMETHODIMP GetStreamState(MF_STREAM_STATE* state) override;

    STDMETHODIMP KsProperty(void* property, ULONG propertyLength, void* data, ULONG dataLength, ULONG* bytesReturned) override;
    STDMETHODIMP KsMethod(void* method, ULONG methodLength, void* data, ULONG dataLength, ULONG* bytesReturned) override;
    STDMETHODIMP KsEvent(void* event, ULONG eventLength, void* data, ULONG dataLength, ULONG* bytesReturned) override;

    HRESULT Start(const PROPVARIANT* startPosition);
    HRESULT Stop();
    void Shutdown();
    IMFStreamDescriptor* Descriptor() const { return descriptor_; }

private:
    void Run();
    HRESULT Deliver(IUnknown* token, const uint8_t* nv12);
    void ConsumeFrameSignal();
    bool AttachBus();
    void ClearRequests();

    long refCount_ = 1;
    std::mutex mutex_;
    IMFMediaEventQueue* queue_ = nullptr;
    MediaSource* source_ = nullptr;
    IMFStreamDescriptor* descriptor_ = nullptr;
    std::queue<IUnknown*> requests_;
    std::thread thread_;
    HANDLE wake_ = nullptr;
    MF_STREAM_STATE state_ = MF_STREAM_STATE_STOPPED;
    bool stop_ = false;
    FrameBus bus_;
};

}  // namespace vcam
