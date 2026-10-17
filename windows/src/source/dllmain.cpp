#include "MediaSource.h"
#include "SourceActivate.h"

#include "camera/CameraIds.h"

#include <atomic>

namespace {

std::atomic<long> g_locks{0};

class ClassFactory : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** object) override {
        if (object == nullptr) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *object = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(&refCount_));
    }

    STDMETHODIMP_(ULONG) Release() override {
        const ULONG count = static_cast<ULONG>(InterlockedDecrement(&refCount_));
        if (count == 0) {
            delete this;
        }
        return count;
    }

    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** object) override {
        if (object == nullptr) {
            return E_POINTER;
        }
        *object = nullptr;
        if (outer != nullptr) {
            return CLASS_E_NOAGGREGATION;
        }
        auto* activate = new vcam::SourceActivate();
        HRESULT hr = activate->QueryInterface(riid, object);
        if (hr == E_NOINTERFACE) {
            hr = activate->ActivateObject(riid, object);
        }
        activate->Release();
        return hr;
    }

    STDMETHODIMP LockServer(BOOL lock) override {
        if (lock) {
            g_locks.fetch_add(1);
        } else {
            g_locks.fetch_sub(1);
        }
        return S_OK;
    }

private:
    long refCount_ = 1;
};

}  // namespace

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) {
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    *object = nullptr;
    if (clsid != vcam::kSourceClsid) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    auto* factory = new ClassFactory();
    const HRESULT hr = factory->QueryInterface(riid, object);
    factory->Release();
    return hr;
}

STDAPI DllCanUnloadNow() {
    return (vcam::ObjectCount() == 0 && g_locks.load() == 0) ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&DllRegisterServer),
            &module)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    wchar_t path[MAX_PATH] = {};
    if (GetModuleFileNameW(module, path, MAX_PATH) == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    wchar_t key[160] = L"Software\\Classes\\CLSID\\";
    wcscat_s(key, vcam::kSourceClsidText);
    wcscat_s(key, L"\\InprocServer32");
    HKEY registry = nullptr;
    const LSTATUS opened = RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &registry, nullptr);
    if (opened != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(opened);
    }
    const DWORD bytes = static_cast<DWORD>((wcslen(path) + 1) * sizeof(wchar_t));
    RegSetValueExW(registry, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(path), bytes);
    const wchar_t* model = L"Both";
    RegSetValueExW(
        registry,
        L"ThreadingModel",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(model),
        static_cast<DWORD>((wcslen(model) + 1) * sizeof(wchar_t))
    );
    RegCloseKey(registry);
    return S_OK;
}

STDAPI DllUnregisterServer() {
    wchar_t key[128] = L"Software\\Classes\\CLSID\\";
    wcscat_s(key, vcam::kSourceClsidText);
    RegDeleteTreeW(HKEY_CURRENT_USER, key);
    return S_OK;
}
