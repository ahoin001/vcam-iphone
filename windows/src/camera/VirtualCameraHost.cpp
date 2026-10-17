#include "VirtualCameraHost.h"

#include "CameraIds.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfvirtualcamera.h>
#include <shlobj.h>

#include <cstdint>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#pragma comment(lib, "shell32.lib")

namespace vcam {
namespace {

constexpr wchar_t kInstallFolderName[] = L"VCamdroid";
constexpr wchar_t kInstalledPrefix[] = L"VCamSource-";

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

bool FileExists(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring ClassKey() {
    return std::wstring(L"Software\\Classes\\CLSID\\") + kSourceClsidText;
}

// A content hash names the installed copy, so a new release never has to overwrite a DLL
// the camera services still have loaded.
bool ContentTag(const std::wstring& path, std::wstring& tag) {
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        return false;
    }
    const std::vector<char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (bytes.empty()) {
        return false;
    }
    uint64_t hash = 14695981039346656037ull;
    for (const char byte : bytes) {
        hash ^= static_cast<uint8_t>(byte);
        hash *= 1099511628211ull;
    }
    wchar_t text[17] = {};
    swprintf_s(text, L"%016llX", static_cast<unsigned long long>(hash));
    tag = text;
    return true;
}

std::wstring InstallFolder() {
    PWSTR programFiles = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &programFiles)) || programFiles == nullptr) {
        if (programFiles != nullptr) {
            CoTaskMemFree(programFiles);
        }
        return L"";
    }
    std::wstring folder = std::wstring(programFiles) + L"\\" + kInstallFolderName;
    CoTaskMemFree(programFiles);
    return folder;
}

std::wstring ExpectedInstallPath(const std::wstring& source) {
    std::wstring tag;
    const std::wstring folder = InstallFolder();
    if (folder.empty() || !ContentTag(source, tag)) {
        return L"";
    }
    return folder + L"\\" + kInstalledPrefix + tag + L".dll";
}

std::wstring MachineRegisteredPath() {
    const std::wstring key = ClassKey() + L"\\InprocServer32";
    wchar_t value[MAX_PATH * 2] = {};
    DWORD bytes = sizeof(value);
    const LSTATUS read = RegGetValueW(HKEY_LOCAL_MACHINE, key.c_str(), nullptr, RRF_RT_REG_SZ, nullptr, value, &bytes);
    if (read != ERROR_SUCCESS) {
        return L"";
    }
    return value;
}

// Earlier builds registered the source for this user only. The camera services never read
// that copy, and it points at the user's download folder, so remove it.
void RemoveUserRegistration() {
    RegDeleteTreeW(HKEY_CURRENT_USER, ClassKey().c_str());
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

std::wstring Code(HRESULT error) {
    wchar_t code[16] = {};
    swprintf_s(code, L"%08X", static_cast<unsigned>(error));
    return code;
}

bool ReachFailure(HRESULT error) {
    return error == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) ||
           error == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
           error == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND) ||
           error == E_ACCESSDENIED ||
           error == REGDB_E_CLASSNOTREG ||
           error == E_NOINTERFACE;
}

void NoteListingFailure(CameraSetup& setup, HRESULT error) {
    if (ReachFailure(error)) {
        setup.needsInstall = true;
        setup.detail = L"The Windows camera services could not load VCamdroid (error " + Code(error) +
            L"). Click Install camera and approve the Windows prompt to reinstall it.";
        return;
    }
    setup.detail = L"Windows 11 is installed, but it did not add VCamdroid to the camera list (error " + Code(error) +
        L"). The picture in this window still works. Other apps will not see VCamdroid until Windows lists it.";
}

}  // namespace

CameraSetup PrepareVirtualCamera() {
    CameraSetup setup;
    setup.windows11 = Windows11OrNewer() && LoadLibraryW(L"mfsensorgroup.dll") != nullptr;
    const std::wstring source = ExeDirectory() + L"\\VCamSource.dll";
    setup.sourceDll = FileExists(source);
    if (!setup.windows11) {
        setup.detail = L"This PC needs Windows 11 before VCamdroid appears in the camera list. The desk still connects, and OBS can use a DroidCam source.";
        return setup;
    }
    if (!setup.sourceDll) {
        setup.detail = L"VCamSource.dll is missing next to VCamPanel.exe, so the camera cannot be installed.";
        return setup;
    }
    RemoveUserRegistration();

    const std::wstring expected = ExpectedInstallPath(source);
    const std::wstring registered = MachineRegisteredPath();
    setup.installed = !expected.empty() && FileExists(expected) && _wcsicmp(registered.c_str(), expected.c_str()) == 0;
    if (!setup.installed) {
        setup.needsInstall = true;
        setup.detail = registered.empty()
            ? L"Windows needs a one-time install of the camera. Click Install camera and approve the Windows prompt."
            : L"This release has a newer camera component. Click Install camera and approve the Windows prompt to update it.";
        return setup;
    }

    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET))) {
        setup.detail = L"Windows did not start the camera pipeline. VCamdroid was not added.";
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
        NoteListingFailure(setup, created);
        return setup;
    }
    const HRESULT started = camera->Start(nullptr);
    camera->Release();
    if (FAILED(started) && !CameraIsListed()) {
        NoteListingFailure(setup, started);
        return setup;
    }
    setup.listed = true;
    return setup;
}

HRESULT InstallCameraSource() {
    const std::wstring source = ExeDirectory() + L"\\VCamSource.dll";
    if (!FileExists(source)) {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    const std::wstring folder = InstallFolder();
    const std::wstring target = ExpectedInstallPath(source);
    if (folder.empty() || target.empty()) {
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    }
    if (!CreateDirectoryW(folder.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!FileExists(target) && !CopyFileW(source.c_str(), target.c_str(), FALSE)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    // A downloaded zip marks its files as from the internet. The services refuse that mark.
    DeleteFileW((target + L":Zone.Identifier").c_str());

    const std::wstring key = ClassKey() + L"\\InprocServer32";
    HKEY registry = nullptr;
    LSTATUS status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, nullptr, 0, KEY_SET_VALUE, nullptr, &registry, nullptr);
    if (status != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(status);
    }
    status = RegSetValueExW(registry, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(target.c_str()),
        static_cast<DWORD>((target.size() + 1) * sizeof(wchar_t)));
    if (status == ERROR_SUCCESS) {
        const wchar_t model[] = L"Both";
        status = RegSetValueExW(registry, L"ThreadingModel", 0, REG_SZ, reinterpret_cast<const BYTE*>(model), sizeof(model));
    }
    RegCloseKey(registry);
    if (status != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(status);
    }

    // Older copies stay until the camera services let go of them. Remove what is free now.
    WIN32_FIND_DATAW found{};
    const std::wstring pattern = folder + L"\\" + kInstalledPrefix + L"*.dll";
    const HANDLE search = FindFirstFileW(pattern.c_str(), &found);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            const std::wstring path = folder + L"\\" + found.cFileName;
            if (_wcsicmp(path.c_str(), target.c_str()) != 0) {
                DeleteFileW(path.c_str());
            }
        } while (FindNextFileW(search, &found));
        FindClose(search);
    }
    return S_OK;
}

}  // namespace vcam
