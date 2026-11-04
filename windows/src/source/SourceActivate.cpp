#include "SourceActivate.h"

#include "MediaSource.h"

#include <mfapi.h>

namespace vcam {

SourceActivate::SourceActivate() {
    TrackObject(1);
    MFCreateAttributes(&attributes_, 4);
}

SourceActivate::~SourceActivate() {
    if (attributes_ != nullptr) {
        attributes_->Release();
        attributes_ = nullptr;
    }
    TrackObject(-1);
}

STDMETHODIMP SourceActivate::QueryInterface(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IMFAttributes || riid == IID_IMFActivate) {
        *object = static_cast<IMFActivate*>(this);
        AddRef();
        return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) SourceActivate::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&refCount_));
}

STDMETHODIMP_(ULONG) SourceActivate::Release() {
    const ULONG count = static_cast<ULONG>(InterlockedDecrement(&refCount_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP SourceActivate::ActivateObject(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    *object = nullptr;
    if (attributes_ == nullptr) {
        return E_OUTOFMEMORY;
    }
    auto* source = new MediaSource();
    HRESULT hr = source->Initialize(attributes_);
    if (SUCCEEDED(hr)) {
        hr = source->QueryInterface(riid, object);
    }
    source->Release();
    return hr;
}

STDMETHODIMP SourceActivate::ShutdownObject() {
    return E_NOTIMPL;
}

STDMETHODIMP SourceActivate::DetachObject() {
    return E_NOTIMPL;
}

STDMETHODIMP SourceActivate::GetItem(REFGUID key, PROPVARIANT* value) {
    return attributes_->GetItem(key, value);
}

STDMETHODIMP SourceActivate::GetItemType(REFGUID key, MF_ATTRIBUTE_TYPE* type) {
    return attributes_->GetItemType(key, type);
}

STDMETHODIMP SourceActivate::CompareItem(REFGUID key, REFPROPVARIANT value, BOOL* result) {
    return attributes_->CompareItem(key, value, result);
}

STDMETHODIMP SourceActivate::Compare(IMFAttributes* theirs, MF_ATTRIBUTES_MATCH_TYPE type, BOOL* result) {
    return attributes_->Compare(theirs, type, result);
}

STDMETHODIMP SourceActivate::GetUINT32(REFGUID key, UINT32* value) {
    return attributes_->GetUINT32(key, value);
}

STDMETHODIMP SourceActivate::GetUINT64(REFGUID key, UINT64* value) {
    return attributes_->GetUINT64(key, value);
}

STDMETHODIMP SourceActivate::GetDouble(REFGUID key, double* value) {
    return attributes_->GetDouble(key, value);
}

STDMETHODIMP SourceActivate::GetGUID(REFGUID key, GUID* value) {
    return attributes_->GetGUID(key, value);
}

STDMETHODIMP SourceActivate::GetStringLength(REFGUID key, UINT32* length) {
    return attributes_->GetStringLength(key, length);
}

STDMETHODIMP SourceActivate::GetString(REFGUID key, LPWSTR value, UINT32 size, UINT32* length) {
    return attributes_->GetString(key, value, size, length);
}

STDMETHODIMP SourceActivate::GetAllocatedString(REFGUID key, LPWSTR* value, UINT32* length) {
    return attributes_->GetAllocatedString(key, value, length);
}

STDMETHODIMP SourceActivate::GetBlobSize(REFGUID key, UINT32* size) {
    return attributes_->GetBlobSize(key, size);
}

STDMETHODIMP SourceActivate::GetBlob(REFGUID key, UINT8* buffer, UINT32 size, UINT32* written) {
    return attributes_->GetBlob(key, buffer, size, written);
}

STDMETHODIMP SourceActivate::GetAllocatedBlob(REFGUID key, UINT8** buffer, UINT32* size) {
    return attributes_->GetAllocatedBlob(key, buffer, size);
}

STDMETHODIMP SourceActivate::GetUnknown(REFGUID key, REFIID riid, LPVOID* object) {
    return attributes_->GetUnknown(key, riid, object);
}

STDMETHODIMP SourceActivate::SetItem(REFGUID key, REFPROPVARIANT value) {
    return attributes_->SetItem(key, value);
}

STDMETHODIMP SourceActivate::DeleteItem(REFGUID key) {
    return attributes_->DeleteItem(key);
}

STDMETHODIMP SourceActivate::DeleteAllItems() {
    return attributes_->DeleteAllItems();
}

STDMETHODIMP SourceActivate::SetUINT32(REFGUID key, UINT32 value) {
    return attributes_->SetUINT32(key, value);
}

STDMETHODIMP SourceActivate::SetUINT64(REFGUID key, UINT64 value) {
    return attributes_->SetUINT64(key, value);
}

STDMETHODIMP SourceActivate::SetDouble(REFGUID key, double value) {
    return attributes_->SetDouble(key, value);
}

STDMETHODIMP SourceActivate::SetGUID(REFGUID key, REFGUID value) {
    return attributes_->SetGUID(key, value);
}

STDMETHODIMP SourceActivate::SetString(REFGUID key, LPCWSTR value) {
    return attributes_->SetString(key, value);
}

STDMETHODIMP SourceActivate::SetBlob(REFGUID key, const UINT8* buffer, UINT32 size) {
    return attributes_->SetBlob(key, buffer, size);
}

STDMETHODIMP SourceActivate::SetUnknown(REFGUID key, IUnknown* value) {
    return attributes_->SetUnknown(key, value);
}

STDMETHODIMP SourceActivate::LockStore() {
    return attributes_->LockStore();
}

STDMETHODIMP SourceActivate::UnlockStore() {
    return attributes_->UnlockStore();
}

STDMETHODIMP SourceActivate::GetCount(UINT32* count) {
    return attributes_->GetCount(count);
}

STDMETHODIMP SourceActivate::GetItemByIndex(UINT32 index, GUID* key, PROPVARIANT* value) {
    return attributes_->GetItemByIndex(index, key, value);
}

STDMETHODIMP SourceActivate::CopyAllItems(IMFAttributes* destination) {
    return attributes_->CopyAllItems(destination);
}

}  // namespace vcam
