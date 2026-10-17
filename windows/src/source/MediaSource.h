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

class MediaStream;

class MediaSource : public IMFMediaSource {
public:
    MediaSource();
    virtual ~MediaSource();

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

private:
    HRESULT CreateDescriptor(IMFPresentationDescriptor** descriptor);

    long refCount_ = 1;
    std::mutex mutex_;
    IMFMediaEventQueue* queue_ = nullptr;
    MediaStream* stream_ = nullptr;
    bool started_ = false;
    bool shutdown_ = false;
    bool sentStream_ = false;
};

class MediaStream : public IMFMediaStream {
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

    HRESULT Start(const PROPVARIANT* startPosition);
    HRESULT Stop();
    IMFStreamDescriptor* Descriptor() const { return descriptor_; }

private:
    void Run();
    HRESULT Deliver(IUnknown* token);
    bool AttachBus();

    long refCount_ = 1;
    std::mutex mutex_;
    IMFMediaEventQueue* queue_ = nullptr;
    MediaSource* source_ = nullptr;
    IMFStreamDescriptor* descriptor_ = nullptr;
    std::queue<IUnknown*> requests_;
    std::thread thread_;
    HANDLE wake_ = nullptr;
    bool started_ = false;
    bool stop_ = false;
    LONGLONG frameIndex_ = 0;
    FrameBus bus_;
};

}  // namespace vcam
