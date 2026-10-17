#include "VirtualCameraHost.h"

#include "CameraIds.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfvirtualcamera.h>

#include <string>

namespace vcam {
namespace {

bool Windows11OrNewer() {
    using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        return false;
    }
    auto* version = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (version == nullptr) {
        return false;
    }
    OSVERSIONINFOW info{};
    info.dwOSVersionInfoSize = sizeof(info);
    if (version(&info) != 0) {
        return false;
    }
    return info.dwMajorVersion > 10 || (info.dwMajorVersion == 10 && info.dwBuildNumber >= 22000);
}

std::wstring ExeDirectory() {
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return L"";
    }
    std::wstring folder(path, length);
    const auto slash = folder.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L"";
    }
    return folder.substr(0, slash);
}

bool RegisterSource(const std::wstring& dllPath) {
    wchar_t key[160] = L"Software\\Classes\\CLSID\\";
    wcscat_s(key, kSourceClsidText);
    wcscat_s(key, L"\\InprocServer32");
    HKEY registry = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &registry, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const DWORD pathBytes = static_cast<DWORD>((dllPath.size() + 1) * sizeof(wchar_t));
    const LSTATUS pathWritten = RegSetValueExW(registry, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(dllPath.c_str()), pathBytes);
    const wchar_t* model = L"Both";
    const LSTATUS modelWritten = RegSetValueExW(
        registry,
        L"ThreadingModel",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(model),
        static_cast<DWORD>((wcslen(model) + 1) * sizeof(wchar_t))
    );
    RegCloseKey(registry);
    return pathWritten == ERROR_SUCCESS && modelWritten == ERROR_SUCCESS;
}

std::wstring ListingFailure(HRESULT error) {
    wchar_t code[16] = {};
    swprintf_s(code, L"%08X", static_cast<unsigned>(error));
    return L"Windows 11 is installed, but it did not add VCamdroid to the camera list (error "
        + std::wstring(code)
        + L"). The picture in this window still works. Other apps will not see VCamdroid until Windows lists it.";
}

bool CameraIsListed() {
    IMFAttributes* attributes = nullptr;
    if (FAILED(MFCreateAttributes(&attributes, 1)) || attributes == nullptr) {
        return false;
    }
    attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    IMFActivate** devices = nullptr;
    UINT32 count = 0;
    const HRESULT listed = MFEnumDeviceSources(attributes, &devices, &count);
    attributes->Release();
    if (FAILED(listed) || devices == nullptr) {
        return false;
    }
    bool found = false;
    for (UINT32 index = 0; index < count; ++index) {
        wchar_t* name = nullptr;
        UINT32 nameLength = 0;
        if (SUCCEEDED(devices[index]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &nameLength)) && name != nullptr) {
            if (wcscmp(name, kCameraName) == 0) {
                found = true;
            }
            CoTaskMemFree(name);
        }
        devices[index]->Release();
    }
    CoTaskMemFree(devices);
    return found;
}

}  // namespace

CameraSetup PrepareVirtualCamera() {
    CameraSetup setup;
    setup.windows11 = Windows11OrNewer() && LoadLibraryW(L"mfsensorgroup.dll") != nullptr;
    const std::wstring dll = ExeDirectory() + L"\\VCamSource.dll";
    setup.sourceDll = GetFileAttributesW(dll.c_str()) != INVALID_FILE_ATTRIBUTES;
    if (!setup.windows11) {
        setup.detail = L"This PC needs Windows 11 before VCamdroid appears in the camera list. The desk still connects, and OBS can use a DroidCam source.";
        return setup;
    }
    if (!setup.sourceDll) {
        setup.detail = L"VCamSource.dll is missing next to VCamPanel.exe, so the system camera was not registered.";
        return setup;
    }
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET))) {
        setup.detail = L"Windows did not start the camera pipeline. VCamdroid was not registered.";
        return setup;
    }
    if (!RegisterSource(dll)) {
        setup.detail = L"Windows did not register the VCamdroid camera for this user.";
        return setup;
    }
    if (CameraIsListed()) {
        setup.listed = true;
        return setup;
    }

    IMFVirtualCamera* camera = nullptr;
    const HRESULT created = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_System,
        MFVirtualCameraAccess_CurrentUser,
        kCameraName,
        kSourceClsidText,
        nullptr,
        0,
        &camera
    );
    if (FAILED(created) || camera == nullptr) {
        if (CameraIsListed()) {
            setup.listed = true;
            return setup;
        }
        setup.detail = ListingFailure(created);
        return setup;
    }
    const HRESULT started = camera->Start(nullptr);
    camera->Release();
    if (FAILED(started) && !CameraIsListed()) {
        setup.detail = ListingFailure(started);
        return setup;
    }
    setup.listed = true;
    return setup;
}

}  // namespace vcam
