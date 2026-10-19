#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "panel_update.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

#ifndef PANEL_VERSION
#define PANEL_VERSION dev
#endif

#define VCAM_STR_IMPL(x) #x
#define VCAM_STR(x) VCAM_STR_IMPL(x)

namespace vcam {
namespace {

constexpr wchar_t kHost[] = L"api.github.com";
constexpr wchar_t kLatestPath[] = L"/repos/ahoin001/vcam-iphone/releases/latest";
constexpr char kZipName[] = "VCamdroid-Windows-x64.zip";
constexpr wchar_t kUserAgent[] = L"VCamPanel";

std::wstring Wide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) {
        return L"";
    }
    std::wstring wide(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), count);
    return wide;
}

std::string Narrow(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int count = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) {
        return {};
    }
    std::string bytes(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), bytes.data(), count, nullptr, nullptr);
    return bytes;
}

std::string JsonText(const std::string& json, const char* key, size_t from = 0) {
    const std::string needle = std::string("\"") + key + "\"";
    const auto at = json.find(needle, from);
    if (at == std::string::npos) {
        return {};
    }
    auto colon = json.find(':', at + needle.size());
    if (colon == std::string::npos) {
        return {};
    }
    ++colon;
    while (colon < json.size() && (json[colon] == ' ' || json[colon] == '\t' || json[colon] == '\n' || json[colon] == '\r')) {
        ++colon;
    }
    if (colon >= json.size() || json[colon] != '"') {
        return {};
    }
    ++colon;
    std::string value;
    while (colon < json.size() && json[colon] != '"') {
        if (json[colon] == '\\' && colon + 1 < json.size()) {
            value.push_back(json[colon + 1]);
            colon += 2;
            continue;
        }
        value.push_back(json[colon]);
        ++colon;
    }
    return value;
}

bool JsonFlag(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\"";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return false;
    }
    auto colon = json.find(':', at + needle.size());
    if (colon == std::string::npos) {
        return false;
    }
    ++colon;
    while (colon < json.size() && (json[colon] == ' ' || json[colon] == '\t')) {
        ++colon;
    }
    return json.compare(colon, 4, "true") == 0;
}

bool ParseVersion(const std::string& tag, int& major, int& minor, int& patch) {
    std::string text = tag;
    if (!text.empty() && (text[0] == 'v' || text[0] == 'V')) {
        text.erase(text.begin());
    }
    if (text.find('-') != std::string::npos) {
        return false;
    }
    major = 0;
    minor = 0;
    patch = 0;
    return sscanf_s(text.c_str(), "%d.%d.%d", &major, &minor, &patch) >= 1;
}

bool Newer(const std::string& latest, const std::string& current) {
    int la = 0, lb = 0, lc = 0, ca = 0, cb = 0, cc = 0;
    if (!ParseVersion(latest, la, lb, lc)) {
        return false;
    }
    if (current == "dev" || !ParseVersion(current, ca, cb, cc)) {
        return true;
    }
    if (la != ca) {
        return la > ca;
    }
    if (lb != cb) {
        return lb > cb;
    }
    return lc > cc;
}

bool CrackUrl(const std::wstring& url, bool& secure, std::wstring& host, std::wstring& path, INTERNET_PORT& port) {
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t hostBuf[256] = {};
    wchar_t pathBuf[2048] = {};
    parts.lpszHostName = hostBuf;
    parts.dwHostNameLength = 256;
    parts.lpszUrlPath = pathBuf;
    parts.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
        return false;
    }
    secure = parts.nScheme == INTERNET_SCHEME_HTTPS;
    host.assign(hostBuf);
    path.assign(pathBuf);
    if (parts.dwExtraInfoLength > 0 && parts.lpszExtraInfo != nullptr) {
        path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    }
    port = parts.nPort;
    return !host.empty() && !path.empty();
}

bool HttpGet(const std::wstring& host, INTERNET_PORT port, bool secure, const std::wstring& path, std::string& body,
             const std::wstring& destFile) {
    HINTERNET session = WinHttpOpen(kUserAgent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        return false;
    }
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));
    DWORD timeout = 20000;
    WinHttpSetTimeouts(session, timeout, timeout, timeout, timeout);
    HINTERNET connect = WinHttpConnect(session, host.c_str(), port, 0);
    if (connect == nullptr) {
        WinHttpCloseHandle(session);
        return false;
    }
    DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (request == nullptr) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }
    const wchar_t* headers = L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
    const bool sent = WinHttpSendRequest(request, headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                      WinHttpReceiveResponse(request, nullptr);
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!sent || !WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                      WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX) ||
        status < 200 || status >= 300) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }
    std::ofstream file;
    if (!destFile.empty()) {
        file.open(destFile.c_str(), std::ios::binary | std::ios::trunc);
        if (!file) {
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return false;
        }
    }
    constexpr size_t kMax = 80u * 1024u * 1024u;
    size_t total = 0;
    bool ok = true;
    while (ok) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            ok = false;
            break;
        }
        if (available == 0) {
            break;
        }
        std::vector<char> chunk(available);
        DWORD read = 0;
        if (!WinHttpReadData(request, chunk.data(), available, &read) || read == 0) {
            break;
        }
        total += read;
        if (total > kMax) {
            ok = false;
            break;
        }
        if (file.is_open()) {
            file.write(chunk.data(), static_cast<std::streamsize>(read));
            if (!file) {
                ok = false;
                break;
            }
        } else {
            body.append(chunk.data(), read);
        }
    }
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    if (file.is_open()) {
        file.close();
        if (!ok) {
            DeleteFileW(destFile.c_str());
        }
    }
    return ok;
}

bool HttpGetUrl(const std::string& url, std::string& body, const std::wstring& destFile) {
    bool secure = true;
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
    if (!CrackUrl(Wide(url), secure, host, path, port)) {
        return false;
    }
    return HttpGet(host, port, secure, path, body, destFile);
}

std::wstring TempRoot() {
    wchar_t folder[MAX_PATH] = {};
    const DWORD length = GetTempPathW(MAX_PATH, folder);
    if (length == 0 || length >= MAX_PATH) {
        return L"";
    }
    std::wstring root(folder);
    if (!root.empty() && root.back() == L'\\') {
        root.pop_back();
    }
    root += L"\\vcam-update";
    CreateDirectoryW(root.c_str(), nullptr);
    return root;
}

void ClearFolder(const std::wstring& folder) {
    WIN32_FIND_DATAW found{};
    const HANDLE search = FindFirstFileW((folder + L"\\*").c_str(), &found);
    if (search == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (wcscmp(found.cFileName, L".") == 0 || wcscmp(found.cFileName, L"..") == 0) {
            continue;
        }
        const std::wstring path = folder + L"\\" + found.cFileName;
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ClearFolder(path);
            RemoveDirectoryW(path.c_str());
        } else {
            SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(path.c_str());
        }
    } while (FindNextFileW(search, &found));
    FindClose(search);
}

bool RunHidden(const std::wstring& exe, std::wstring command) {
    STARTUPINFOW info{};
    info.cb = sizeof(info);
    info.dwFlags = STARTF_USESHOWWINDOW;
    info.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &info,
                        &process)) {
        return false;
    }
    WaitForSingleObject(process.hProcess, 60000);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return code == 0;
}

}  // namespace

const char* CurrentVersion() {
    return VCAM_STR(PANEL_VERSION);
}

bool IsDevBuild() {
    return std::strcmp(VCAM_STR(PANEL_VERSION), "dev") == 0;
}

std::wstring ExeFolder() {
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

UpdateOffer CheckLatest() {
    UpdateOffer offer;
    std::string body;
    if (!HttpGet(kHost, INTERNET_DEFAULT_HTTPS_PORT, true, kLatestPath, body, L"")) {
        offer.detail = L"GitHub could not be reached. Check this PC\u2019s network, then click Check for updates.";
        return offer;
    }
    offer.ok = true;
    const std::string tag = JsonText(body, "tag_name");
    if (tag.empty() || JsonFlag(body, "prerelease") || tag.find('-') != std::string::npos) {
        offer.detail = Wide(std::string("This build is ") + CurrentVersion() + ".");
        return offer;
    }
    offer.tag = tag;
    const auto zipAt = body.find(kZipName);
    if (zipAt != std::string::npos) {
        offer.url = JsonText(body, "browser_download_url", zipAt);
    }
    if (offer.url.empty()) {
        offer.url = "https://github.com/ahoin001/vcam-iphone/releases/latest/download/VCamdroid-Windows-x64.zip";
    }
    if (Newer(tag, CurrentVersion())) {
        offer.available = true;
        offer.detail = Wide("A newer VCamPanel is ready (" + tag +
                            "). Update restarts this window and stops the picture.");
    } else {
        offer.detail = Wide(std::string("This build is ") + CurrentVersion() + ". It is current.");
    }
    return offer;
}

UpdateStage PrepareUpdate(const std::string& url) {
    UpdateStage stage;
    const std::wstring root = TempRoot();
    if (root.empty()) {
        stage.detail = L"Windows did not give a temp folder for the download.";
        return stage;
    }
    const std::wstring folder = root + L"\\stage";
    CreateDirectoryW(folder.c_str(), nullptr);
    ClearFolder(folder);
    const std::wstring zip = root + L"\\VCamdroid-Windows-x64.zip";
    DeleteFileW(zip.c_str());
    std::string unused;
    if (!HttpGetUrl(url, unused, zip)) {
        stage.detail = L"The new companion did not download. Check this PC\u2019s network, or GitHub may be blocked.";
        return stage;
    }
    wchar_t windows[MAX_PATH] = {};
    GetSystemDirectoryW(windows, MAX_PATH);
    const std::wstring tar = std::wstring(windows) + L"\\tar.exe";
    std::wstring command = L"\"" + tar + L"\" -xf \"" + zip + L"\" -C \"" + folder + L"\"";
    if (!RunHidden(tar, command)) {
        stage.detail = L"Windows could not unpack the update zip.";
        return stage;
    }
    if (GetFileAttributesW((folder + L"\\VCamPanel.exe").c_str()) == INVALID_FILE_ATTRIBUTES) {
        stage.detail = L"The update zip did not contain VCamPanel.exe.";
        return stage;
    }
    const std::wstring script = root + L"\\apply.cmd";
    std::ofstream out(script.c_str(), std::ios::binary | std::ios::trunc);
    out << "@echo off\r\nsetlocal\r\n"
           "set \"SRC=%~1\"\r\nset \"DST=%~2\"\r\nset \"EXE=%~3\"\r\n"
           ":wait\r\ntimeout /t 1 /nobreak >nul\r\n"
           "tasklist /FI \"IMAGENAME eq VCamPanel.exe\" | find /I \"VCamPanel.exe\" >nul\r\n"
           "if not errorlevel 1 goto wait\r\n"
           "copy /Y \"%SRC%\\VCamPanel.exe\" \"%DST%\\\" >nul\r\n"
           "copy /Y \"%SRC%\\VCamSource.dll\" \"%DST%\\\" >nul\r\n"
           "if exist \"%SRC%\\usbmux_forward.exe\" copy /Y \"%SRC%\\usbmux_forward.exe\" \"%DST%\\\" >nul\r\n"
           "start \"\" \"%EXE%\"\r\n";
    const bool wrote = static_cast<bool>(out);
    out.close();
    if (!wrote) {
        stage.detail = L"The updater script could not be written.";
        return stage;
    }
    stage.ok = true;
    stage.folder = folder;
    return stage;
}

bool LaunchUpdater(const std::wstring& stageFolder) {
    const std::wstring dest = ExeFolder();
    if (dest.empty() || stageFolder.empty()) {
        return false;
    }
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    std::wstring script = temp;
    if (!script.empty() && script.back() == L'\\') {
        script.pop_back();
    }
    script += L"\\vcam-update\\apply.cmd";
    wchar_t system[MAX_PATH] = {};
    GetSystemDirectoryW(system, MAX_PATH);
    const std::wstring cmd = std::wstring(system) + L"\\cmd.exe";
    const std::wstring exe = dest + L"\\VCamPanel.exe";
    std::wstring command = L"\"" + cmd + L"\" /C \"\"" + script + L"\" \"" + stageFolder + L"\" \"" + dest + L"\" \"" +
                           exe + L"\"\"";
    STARTUPINFOW info{};
    info.cb = sizeof(info);
    info.dwFlags = STARTF_USESHOWWINDOW;
    info.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &info,
                        &process)) {
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

}  // namespace vcam
