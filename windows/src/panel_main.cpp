#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <wrl.h>
#include <WebView2.h>

#include "frame_advice.h"
#include "guide_photos.h"
#include "mdns_browse.h"
#include "panel_shell.h"
#include "panel_update.h"
#include "obs_tally.h"
#include "usbmux.h"
#include "camera/Pipeline.h"
#include "camera/PreviewSurface.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr UINT kStatusTimer = 1;
constexpr UINT kScanTimer = 2;
constexpr UINT kDeskRetryTimer = 3;
constexpr UINT kStatusIntervalMs = 1000;
constexpr UINT kScanIntervalMs = 4000;
constexpr UINT kDeskRetryMs = 2500;
constexpr UINT kScanDoneMessage = WM_APP + 1;
constexpr UINT kPollDoneMessage = WM_APP + 2;
constexpr UINT kInstallDoneMessage = WM_APP + 3;
constexpr UINT kTipDoneMessage = WM_APP + 4;
constexpr UINT kUpdateDoneMessage = WM_APP + 5;
constexpr wchar_t kInstallSwitch[] = L"--install-camera";
constexpr int kToolbarHeight = 60;
constexpr int kTipsTrayHeight = 168;
constexpr int kPaneRadius = 14;
constexpr int kSplitterHit = 10;
constexpr int kMinPreview = 240;
constexpr int kMinDesk = 280;
constexpr double kDefaultSideRatio = 0.30;
constexpr double kDefaultStackRatio = 0.55;
constexpr double kMinDeskRatio = 0.18;
constexpr double kMaxSideRatio = 0.55;
constexpr double kMaxStackRatio = 0.75;
constexpr size_t kMaxDeskLayoutBytes = 1024;
constexpr int kWifiGraceScans = 3;
constexpr int kConnectTimeoutMs = 800;

constexpr COLORREF kBg = RGB(18, 20, 22);
constexpr COREWEBVIEW2_COLOR kOpaqueBg{255, 18, 20, 22};
constexpr COREWEBVIEW2_COLOR kClearBg{0, 0, 0, 0};

// Raw DWM values so older SDK headers still build. Windows 10 rejects them and keeps the solid frame.
constexpr DWORD kDwmDarkMode = 20;
constexpr DWORD kDwmCornerPreference = 33;
constexpr DWORD kDwmBackdropType = 38;
constexpr int kDwmRoundCorners = 2;
constexpr int kDwmMicaBackdrop = 2;

constexpr wchar_t kDeskWaitingHtml[] =
    L"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><style>"
    L"html,body{height:100%;margin:0}"
    L"body{display:grid;place-items:center;padding:24px;box-sizing:border-box;background:#121416;color:#9a958c;"
    L"font:13px/1.5 'Segoe UI Variable Text','Segoe UI',system-ui,sans-serif;text-align:center}"
    L"strong{display:block;margin-bottom:4px;color:#f2efe8;font-size:14px;font-weight:600}"
    L"</style></head><body><div><strong>Opening the camera desk</strong>"
    L"Keep VCamdroid open on the studio screen. This retries on its own.</div></body></html>";

enum class Kind { Usb, Wifi };
enum class Tone { Idle, Ready, Busy, Live, Warn, Error };
enum class CheckState { Ready, Waiting, Blocked };

struct Listed {
    Kind kind = Kind::Wifi;
    int usbId = 0;
    std::string ipv4;
    int port = 4747;
    std::wstring name;
    std::wstring detail;
    std::string serial;
    int missed = 0;
};

struct ScanResult {
    std::vector<Listed> devices;
    std::wstring usbError;
};

struct SessionPoll {
    std::string host;
    int port = 0;
    bool ok = false;
    std::string body;
    unsigned long long startedMs = 0;
    unsigned long long finishedMs = 0;
};

struct Pace {
    long long offset = 0;
    bool haveOffset = false;
    unsigned int decodedFrames = 0;
    unsigned long long fpsSampleMs = 0;
    double fps = 0;
    unsigned int repeats = 0;
    bool haveRepeats = false;
    int drops = 0;
    int lastDelay = -1;
    unsigned long long climbSince = 0;
    unsigned long long healthySince = 0;
    std::wstring notice;
    bool repeatsClimbing = false;
};

struct CpuSample {
    unsigned long long idle = 0;
    unsigned long long kernel = 0;
    unsigned long long user = 0;
    int percent = 0;
};

struct UpdateJob {
    bool apply = false;
    bool ok = false;
    bool available = false;
    std::string tag;
    std::string url;
    std::wstring detail;
    std::wstring stage;
};

struct UpdateView {
    bool checked = false;
    bool ok = false;
    bool available = false;
    bool busy = false;
    std::string tag;
    std::string url;
    std::wstring detail;
};

struct TipView {
    std::vector<vcam::Tip> current;
    std::vector<std::string> dismissed;
    std::string doneId;
    unsigned long long doneAt = 0;
    unsigned long long problemSince = 0;
    unsigned long long healthySince = 0;
    int lastPhoneDrops = 0;
    int lastCaptureDrops = 0;
    unsigned long long phoneRecent = 0;
    unsigned long long captureRecent = 0;
    bool open = false;
    bool ready = false;
};

struct StatusView {
    Tone tone = Tone::Idle;
    std::wstring title = L"Starting";
    std::wstring detail;
    std::vector<std::wstring> chips;
};

Pace g_pace;
CpuSample g_cpu;
TipView g_tips;
std::thread g_tipThread;
std::string g_pendingTipId;
UpdateView g_update;
std::thread g_updateThread;

struct Check {
    CheckState state = CheckState::Waiting;
    bool pc = false;
    std::wstring title;
    std::wstring detail;
    const char* action = nullptr;
};

HWND g_window = nullptr;
HWND g_preview = nullptr;
HWND g_deskHost = nullptr;
HWND g_splitter = nullptr;
HBRUSH g_background = nullptr;
vcam::PreviewSurface g_previewSurface;

ComPtr<ICoreWebView2Controller> g_shellController;
ComPtr<ICoreWebView2> g_shell;
ComPtr<ICoreWebView2Controller> g_deskController;
ComPtr<ICoreWebView2> g_desk;
bool g_shellReady = false;
bool g_shellNavigationAllowed = false;
bool g_webViewErrorShown = false;
bool g_mica = false;
bool g_onTop = true;
bool g_sheetOpen = false;
bool g_guideOpen = false;

std::vector<Listed> g_devices;
std::string g_selectedKey;
bool g_userClearedSelection = false;
std::wstring g_usbError;
std::wstring g_connectError;
std::thread g_scanThread;
std::atomic<bool> g_scanning{false};
std::thread g_pollThread;
std::atomic<bool> g_polling{false};
std::thread g_installThread;
bool g_installing = false;
std::wstring g_installNote;
bool g_firstScan = true;

bool g_usbConnected = false;
std::string g_connectedHost;
int g_connectedPort = 4747;
std::wstring g_connectedName;
std::wstring g_deskUrl;
Listed g_lastTarget;
bool g_haveLastTarget = false;
bool g_userDisconnected = false;
bool g_phoneLeft = false;
std::wstring g_phoneLeftDetail;
int g_sessionMisses = 0;
std::wstring g_address;
std::wstring g_obsPassword;
std::wstring g_obsAddress = L"127.0.0.1:4747";
bool g_peaking = false;
bool g_zebra = false;
bool g_loupe = false;
StatusView g_status;
double g_sideRatio = kDefaultSideRatio;
double g_stackRatio = kDefaultStackRatio;
std::wstring g_deskLayout;
bool g_stacked = false;
bool g_splitting = false;
int g_splitOrigin = 0;
double g_splitRatioOrigin = kDefaultSideRatio;
int g_innerW = 0;
int g_innerH = 0;

int Scale(HWND window, int pixels) {
    const UINT dpi = GetDpiForWindow(window);
    return MulDiv(pixels, dpi == 0 ? 96 : static_cast<int>(dpi), 96);
}

bool Connected() {
    return !g_connectedHost.empty();
}

std::wstring Trim(std::wstring text) {
    const auto start = text.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) {
        return L"";
    }
    const auto end = text.find_last_not_of(L" \t\r\n");
    return text.substr(start, end - start + 1);
}

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

std::wstring EnvFolder(const wchar_t* name) {
    wchar_t buffer[MAX_PATH] = {};
    const DWORD length = GetEnvironmentVariableW(name, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return L"";
    }
    return buffer;
}

std::wstring SettingPath(const wchar_t* file) {
    const std::wstring root = EnvFolder(L"APPDATA");
    if (root.empty()) {
        return L"";
    }
    const std::wstring folder = root + L"\\VCamdroid";
    CreateDirectoryW(folder.c_str(), nullptr);
    return folder + L"\\" + file;
}

std::wstring WebViewFolder() {
    const std::wstring root = EnvFolder(L"LOCALAPPDATA");
    if (root.empty()) {
        return L"";
    }
    const std::wstring folder = root + L"\\VCamdroid";
    CreateDirectoryW(folder.c_str(), nullptr);
    const std::wstring webview = folder + L"\\WebView2";
    CreateDirectoryW(webview.c_str(), nullptr);
    return webview;
}

std::wstring LoadSetting(const wchar_t* file) {
    const std::wstring path = SettingPath(file);
    if (path.empty()) {
        return L"";
    }
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        return L"";
    }
    std::string bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    return Trim(Wide(bytes));
}

void SaveSetting(const wchar_t* file, const std::wstring& text) {
    const std::wstring path = SettingPath(file);
    if (path.empty()) {
        return;
    }
    const std::string encoded = Narrow(text);
    const std::wstring temp = path + L".tmp";
    {
        std::ofstream output(temp.c_str(), std::ios::binary | std::ios::trunc);
        output.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
        if (!output) {
            return;
        }
    }
    // Replace in one step so a crash mid-write never leaves a half-written setting.
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str());
    }
}

double& DeskRatio(bool stacked) {
    return stacked ? g_stackRatio : g_sideRatio;
}

double ClampDeskRatio(double ratio, int inner, bool stacked) {
    const double ceiling = stacked ? kMaxStackRatio : kMaxSideRatio;
    ratio = (std::max)(kMinDeskRatio, (std::min)(ceiling, ratio));
    const int minPreview = Scale(g_window, stacked ? 140 : kMinPreview);
    const int minDesk = Scale(g_window, kMinDesk);
    if (inner <= minPreview + minDesk) {
        return ratio;
    }
    const double lo = (std::max)(kMinDeskRatio, static_cast<double>(minDesk) / inner);
    const double hi = (std::min)(ceiling, 1.0 - static_cast<double>(minPreview) / inner);
    if (lo >= hi) {
        return (lo + hi) * 0.5;
    }
    return (std::max)(lo, (std::min)(hi, ratio));
}

double ParseRatio(const std::wstring& text, double fallback, double ceiling) {
    wchar_t* end = nullptr;
    const double value = wcstod(text.c_str(), &end);
    if (end == text.c_str() || !(value > 0.0)) {
        return fallback;
    }
    return (std::max)(kMinDeskRatio, (std::min)(ceiling, value));
}

void LoadDeskRatio() {
    const std::wstring text = LoadSetting(L"panel-split.txt");
    const auto space = text.find(L' ');
    g_sideRatio = ParseRatio(text.substr(0, space), kDefaultSideRatio, kMaxSideRatio);
    g_stackRatio = space == std::wstring::npos
        ? kDefaultStackRatio
        : ParseRatio(text.substr(space + 1), kDefaultStackRatio, kMaxStackRatio);
}

void PersistDeskRatio() {
    wchar_t buffer[64] = {};
    swprintf_s(buffer, L"%.3f %.3f", g_sideRatio, g_stackRatio);
    SaveSetting(L"panel-split.txt", buffer);
}

void SaveWindowPlacement() {
    if (!g_window) {
        return;
    }
    WINDOWPLACEMENT placement{sizeof(placement)};
    if (!GetWindowPlacement(g_window, &placement)) {
        return;
    }
    const RECT& r = placement.rcNormalPosition;
    if (r.right - r.left <= 0 || r.bottom - r.top <= 0) {
        return;
    }
    const bool maximized = placement.showCmd == SW_SHOWMAXIMIZED
        || (placement.showCmd == SW_SHOWMINIMIZED && (placement.flags & WPF_RESTORETOMAXIMIZED) != 0);
    wchar_t buffer[96] = {};
    swprintf_s(buffer, L"%ld %ld %ld %ld %d", r.left, r.top, r.right, r.bottom, maximized ? 1 : 0);
    SaveSetting(L"panel-window.txt", buffer);
}

bool RestoreWindowPlacement(int show) {
    const std::wstring text = LoadSetting(L"panel-window.txt");
    long left = 0, top = 0, right = 0, bottom = 0;
    int maximized = 0;
    if (swscanf_s(text.c_str(), L"%ld %ld %ld %ld %d", &left, &top, &right, &bottom, &maximized) != 5) {
        return false;
    }
    RECT bounds{left, top, right, bottom};
    if (right - left < 200 || bottom - top < 150) {
        return false;
    }
    // A monitor that was unplugged since last run would put the window off screen.
    if (MonitorFromRect(&bounds, MONITOR_DEFAULTTONULL) == nullptr) {
        return false;
    }
    WINDOWPLACEMENT placement{sizeof(placement)};
    const bool launchedMinimized = show == SW_SHOWMINIMIZED || show == SW_MINIMIZE || show == SW_SHOWMINNOACTIVE;
    placement.showCmd = launchedMinimized ? SW_SHOWMINNOACTIVE : (maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
    if (maximized) {
        placement.flags = WPF_RESTORETOMAXIMIZED;
    }
    placement.rcNormalPosition = bounds;
    return SetWindowPlacement(g_window, &placement) != FALSE;
}

bool ValidDeskLayout(const std::wstring& text) {
    if (text.empty() || text.size() > kMaxDeskLayoutBytes) {
        return false;
    }
    for (const wchar_t c : text) {
        const bool allowed = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9')
            || c == L'{' || c == L'}' || c == L'"' || c == L':' || c == L',' || c == L'.';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

std::wstring PercentEncode(const std::wstring& text) {
    static const wchar_t* hex = L"0123456789ABCDEF";
    std::wstring out;
    for (const char c : Narrow(text)) {
        const auto byte = static_cast<unsigned char>(c);
        if ((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') || (byte >= '0' && byte <= '9') || byte == '.') {
            out += static_cast<wchar_t>(byte);
        } else {
            out += L'%';
            out += hex[byte >> 4];
            out += hex[byte & 0xF];
        }
    }
    return out;
}

// The layout rides in the fragment so it never reaches the phone and survives a new IP or USB/Wi-Fi switch.
std::wstring DeskTarget() {
    if (g_deskUrl.empty() || g_deskLayout.empty()) {
        return g_deskUrl;
    }
    return g_deskUrl + L"#layout=" + PercentEncode(g_deskLayout);
}

bool ParseHostPort(const std::wstring& text, std::string& host, int& port) {
    std::wstring value = Trim(text);
    if (value.empty()) {
        return false;
    }
    const auto scheme = value.find(L"://");
    if (scheme != std::wstring::npos) {
        value = value.substr(scheme + 3);
    }
    const auto slash = value.find(L'/');
    if (slash != std::wstring::npos) {
        value = value.substr(0, slash);
    }
    const auto colon = value.rfind(L':');
    port = 4747;
    if (colon != std::wstring::npos) {
        host = Narrow(value.substr(0, colon));
        port = _wtoi(value.substr(colon + 1).c_str());
    } else {
        host = Narrow(value);
    }
    return !host.empty() && port > 0 && port <= 65535;
}

unsigned long long JsonU64(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\":";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return 0;
    }
    return strtoull(json.c_str() + at + needle.size(), nullptr, 10);
}

int JsonInt(const std::string& json, const char* key, int fallback) {
    const std::string needle = std::string("\"") + key + "\":";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return fallback;
    }
    return atoi(json.c_str() + at + needle.size());
}

double JsonNumber(const std::string& json, const char* key, double fallback) {
    const std::string needle = std::string("\"") + key + "\":";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return fallback;
    }
    return strtod(json.c_str() + at + needle.size(), nullptr);
}

std::string JsonString(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\":\"";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return {};
    }
    const auto start = at + needle.size();
    const auto end = json.find('"', start);
    if (end == std::string::npos) {
        return {};
    }
    return json.substr(start, end - start);
}

bool JsonBool(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\":";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return false;
    }
    return json.compare(at + needle.size(), 4, "true") == 0;
}

void AppendJsonString(std::string& out, const std::wstring& text) {
    out += '"';
    for (const char c : Narrow(text)) {
        const auto byte = static_cast<unsigned char>(c);
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (byte < 0x20) {
            char escaped[8];
            snprintf(escaped, sizeof(escaped), "\\u%04x", byte);
            out += escaped;
        } else {
            out += c;
        }
    }
    out += '"';
}

void AppendJsonBool(std::string& out, const char* key, bool value) {
    out += ",\"";
    out += key;
    out += "\":";
    out += value ? "true" : "false";
}

bool ConnectWithTimeout(SOCKET socket, const sockaddr_in& address, int timeoutMs) {
    u_long nonBlocking = 1;
    ioctlsocket(socket, FIONBIO, &nonBlocking);
    bool connected = connect(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
    if (!connected && WSAGetLastError() == WSAEWOULDBLOCK) {
        fd_set writable;
        FD_ZERO(&writable);
        FD_SET(socket, &writable);
        fd_set failed;
        FD_ZERO(&failed);
        FD_SET(socket, &failed);
        timeval wait{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
        connected = select(0, nullptr, &writable, &failed, &wait) > 0 && FD_ISSET(socket, &writable);
    }
    u_long blocking = 0;
    ioctlsocket(socket, FIONBIO, &blocking);
    return connected;
}

bool HttpGet(const std::string& host, int port, const char* path, std::string& body) {
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET) {
        return false;
    }
    DWORD timeout = 800;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        closesocket(socket);
        return false;
    }
    if (!ConnectWithTimeout(socket, address, kConnectTimeoutMs)) {
        closesocket(socket);
        return false;
    }
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\nHost: " << host << ":" << port
            << "\r\nConnection: close\r\n\r\n";
    const std::string blob = request.str();
    if (send(socket, blob.data(), static_cast<int>(blob.size()), 0) <= 0) {
        closesocket(socket);
        return false;
    }
    std::string response;
    char buffer[1024];
    int read = 0;
    while ((read = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, static_cast<size_t>(read));
        if (response.size() > 8192) {
            break;
        }
    }
    closesocket(socket);
    const auto split = response.find("\r\n\r\n");
    if (split == std::string::npos) {
        return false;
    }
    body = response.substr(split + 4);
    return response.find("200") != std::string::npos;
}

bool HttpPut(const std::string& host, int port, const std::string& path) {
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET) {
        return false;
    }
    DWORD timeout = 800;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        closesocket(socket);
        return false;
    }
    if (!ConnectWithTimeout(socket, address, kConnectTimeoutMs)) {
        closesocket(socket);
        return false;
    }
    std::ostringstream request;
    request << "PUT " << path << " HTTP/1.1\r\nHost: " << host << ":" << port
            << "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    const std::string blob = request.str();
    const bool sent = send(socket, blob.data(), static_cast<int>(blob.size()), 0) > 0;
    closesocket(socket);
    return sent;
}

int SampleCpu() {
    FILETIME idle{};
    FILETIME kernel{};
    FILETIME user{};
    if (!GetSystemTimes(&idle, &kernel, &user)) {
        return g_cpu.percent;
    }
    auto ticks = [](FILETIME time) {
        return (static_cast<unsigned long long>(time.dwHighDateTime) << 32) | time.dwLowDateTime;
    };
    const unsigned long long idleNow = ticks(idle);
    const unsigned long long kernelNow = ticks(kernel);
    const unsigned long long userNow = ticks(user);
    if (g_cpu.kernel != 0) {
        const unsigned long long idleDelta = idleNow - g_cpu.idle;
        const unsigned long long totalDelta = (kernelNow - g_cpu.kernel) + (userNow - g_cpu.user);
        if (totalDelta > 0) {
            const int percent = static_cast<int>(100 - (idleDelta * 100) / totalDelta);
            g_cpu.percent = (std::max)(0, (std::min)(100, percent));
        }
    }
    g_cpu.idle = idleNow;
    g_cpu.kernel = kernelNow;
    g_cpu.user = userNow;
    return g_cpu.percent;
}

bool TipDismissed(const std::string& id) {
    return std::find(g_tips.dismissed.begin(), g_tips.dismissed.end(), id) != g_tips.dismissed.end();
}

void ResetTips() {
    if (g_tipThread.joinable()) {
        g_tipThread.join();
    }
    g_tips = {};
    g_cpu = {};
    g_pendingTipId.clear();
}

void CopyText(const std::wstring& text) {
    if (!OpenClipboard(g_window)) {
        return;
    }
    EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory) {
        void* locked = GlobalLock(memory);
        if (locked) {
            memcpy(locked, text.c_str(), bytes);
            GlobalUnlock(memory);
            SetClipboardData(CF_UNICODETEXT, memory);
        } else {
            GlobalFree(memory);
        }
    }
    CloseClipboard();
}

constexpr wchar_t kPhoneCue[] =
    L"On the iPhone, leave VCamdroid open on the camera screen. The picture appears here once the phone starts sending it.";
constexpr wchar_t kUsbAppClosed[] =
    L"USB is connected, but VCamdroid is not on the camera screen.";
constexpr wchar_t kCannotDecode[] =
    L"The PC is receiving the picture, but it cannot decode it. Leave the camera screen open, then click Connect again.";

bool ReceivingRecently(const vcam::LinkProgress& progress) {
    if (progress.lastByteMs == 0) {
        return false;
    }
    return GetTickCount64() - progress.lastByteMs < 3000;
}

bool DecodedRecently(const vcam::LinkProgress& progress) {
    if (progress.lastDecodedMs == 0) {
        return false;
    }
    return GetTickCount64() - progress.lastDecodedMs < 3000;
}

std::string g_reportedLink;

void ReportLink(const std::string& host, int port, const std::string& body) {
    const auto progress = vcam::Pipeline::Shared().Progress();
    const bool phoneSending = JsonBool(body, "streaming") || JsonBool(body, "panelClient");
    std::string link;
    if (phoneSending && ReceivingRecently(progress) && !DecodedRecently(progress)) {
        link = "cannotDecode";
    } else if (progress.state == vcam::LinkState::Live) {
        link = "live";
    } else {
        return;
    }
    if (g_reportedLink == link) {
        return;
    }
    if (HttpPut(host, port, "/v1/session?link=" + link)) {
        g_reportedLink = link;
    }
}

std::string DeviceKey(const Listed& device) {
    if (device.kind == Kind::Usb) {
        if (!device.serial.empty()) {
            return "usb:" + device.serial;
        }
        return "usb-id:" + std::to_string(device.usbId);
    }
    return "wifi:" + device.ipv4 + ":" + std::to_string(device.port);
}

const Listed* FindDevice(const std::string& key) {
    for (const Listed& device : g_devices) {
        if (DeviceKey(device) == key) {
            return &device;
        }
    }
    return nullptr;
}

const char* ToneName(Tone tone) {
    switch (tone) {
    case Tone::Ready: return "ready";
    case Tone::Busy: return "busy";
    case Tone::Live: return "live";
    case Tone::Warn: return "warn";
    case Tone::Error: return "error";
    default: return "idle";
    }
}

const char* CheckName(CheckState state) {
    switch (state) {
    case CheckState::Ready: return "ready";
    case CheckState::Blocked: return "blocked";
    default: return "waiting";
    }
}

std::vector<Check> BuildChecks() {
    const vcam::CameraSetup setup = vcam::Pipeline::Shared().Setup();
    const vcam::LinkState link = vcam::Pipeline::Shared().State();
    std::vector<Check> checks;
    checks.reserve(6);

    Check companion;
    companion.pc = true;
    companion.title = L"VCamPanel";
    if (g_update.available) {
        companion.state = CheckState::Blocked;
        companion.detail = g_update.detail;
        if (!g_update.busy) {
            companion.action = "update";
        }
    } else {
        companion.state = CheckState::Ready;
        if (g_update.busy) {
            companion.detail = g_update.detail.empty() ? L"Checking GitHub for a newer companion." : g_update.detail;
        } else if (g_update.checked && !g_update.ok) {
            companion.detail = g_update.detail.empty()
                ? L"GitHub could not be reached. Click Check for updates to try again."
                : g_update.detail;
        } else {
            companion.detail = g_update.detail.empty()
                ? (vcam::IsDevBuild() ? L"This is an unreleased build."
                                      : Wide(std::string("This build is ") + vcam::CurrentVersion() + "."))
                : g_update.detail;
        }
    }
    checks.push_back(std::move(companion));

    Check windows;
    windows.pc = true;
    windows.title = L"Windows 11";
    if (setup.windows11) {
        windows.state = CheckState::Ready;
        windows.detail = L"This PC is running Windows 11.";
    } else {
        windows.state = CheckState::Blocked;
        windows.detail = L"Other apps can select VCamdroid on Windows 11. This PC is older, so the camera stays out of the list. The desk still connects, and OBS can use a DroidCam source.";
    }
    checks.push_back(std::move(windows));

    Check dll;
    dll.pc = true;
    dll.title = L"Camera component";
    if (setup.sourceDll) {
        dll.state = CheckState::Ready;
        dll.detail = L"VCamSource.dll is next to VCamPanel.exe.";
    } else {
        dll.state = CheckState::Blocked;
        dll.detail = L"VCamSource.dll is missing next to VCamPanel.exe. Unzip the whole release folder and open VCamPanel from that folder.";
    }
    checks.push_back(std::move(dll));

    Check camera;
    camera.pc = true;
    camera.title = L"Camera listed as VCamdroid";
    if (g_installing) {
        camera.detail = L"Approve the Windows prompt to install the camera.";
    } else if (setup.needsInstall) {
        camera.state = CheckState::Blocked;
        camera.detail = g_installNote.empty() ? setup.detail : g_installNote;
        camera.action = "install";
    } else if (setup.listed) {
        camera.state = CheckState::Ready;
        camera.detail = L"Other apps can select VCamdroid.";
    } else if (!setup.windows11) {
        camera.state = CheckState::Blocked;
        camera.detail = L"Listing waits for Windows 11.";
    } else if (!setup.sourceDll) {
        camera.state = CheckState::Blocked;
        camera.detail = L"Registering waits until VCamSource.dll is next to VCamPanel.exe.";
    } else {
        camera.state = CheckState::Blocked;
        camera.detail = setup.detail.empty() ? L"Windows did not list the VCamdroid camera." : setup.detail;
    }
    checks.push_back(std::move(camera));

    const vcam::LinkProgress progress = vcam::Pipeline::Shared().Progress();
    Check phone;
    phone.title = L"Picture from the iPhone";
    if (link == vcam::LinkState::Live) {
        phone.state = CheckState::Ready;
        phone.detail = L"Frames are arriving. The preview is the same picture other apps receive.";
    } else if (!g_connectError.empty()) {
        phone.state = CheckState::Blocked;
        phone.detail = g_connectError;
    } else if (link == vcam::LinkState::Interrupted) {
        phone.state = CheckState::Blocked;
        phone.detail = L"The video connection closed. Click Connect to try again.";
    } else if (Connected() && progress.closedWithoutVideo && g_usbConnected) {
        phone.state = CheckState::Blocked;
        phone.detail = kUsbAppClosed;
    } else if (Connected() && ReceivingRecently(progress) && !DecodedRecently(progress)) {
        phone.state = CheckState::Blocked;
        phone.detail = kCannotDecode;
    } else if (Connected()) {
        phone.detail = kPhoneCue;
    } else {
        phone.detail = L"Pick your iPhone, then Connect.";
    }
    checks.push_back(std::move(phone));

    Check choose;
    choose.title = L"Choose VCamdroid";
    if (link == vcam::LinkState::Live && setup.listed) {
        choose.state = CheckState::Ready;
        choose.detail = L"In NVIDIA Broadcast, OBS, Zoom, Teams, Meet, or Discord, choose the camera named VCamdroid. For blur, set NVIDIA Broadcast\u2019s camera to VCamdroid, then select NVIDIA Broadcast in the other app.";
    } else if (!setup.windows11) {
        choose.state = CheckState::Blocked;
        choose.detail = L"This PC cannot list VCamdroid. The desk still works. OBS can use a DroidCam source.";
    } else {
        choose.detail = L"Ready once the picture is live and VCamdroid is in the camera list.";
    }
    checks.push_back(std::move(choose));
    return checks;
}

std::string BuildState() {
    std::string json = "{\"type\":\"state\"";
    AppendJsonBool(json, "connected", Connected());
    AppendJsonBool(json, "mica", g_mica);
    AppendJsonBool(json, "onTop", g_onTop);
    AppendJsonBool(json, "sheet", g_sheetOpen);
    AppendJsonBool(json, "guide", g_guideOpen);
    AppendJsonBool(json, "scanning", g_scanning.load());
    json += ",\"selected\":";
    AppendJsonString(json, Wide(g_selectedKey));
    json += ",\"address\":";
    AppendJsonString(json, g_address);
    json += ",\"obs\":";
    AppendJsonString(json, g_obsAddress);
    json += ",\"obsPassword\":";
    AppendJsonString(json, g_obsPassword);
    AppendJsonBool(json, "peaking", g_peaking);
    AppendJsonBool(json, "zebra", g_zebra);
    AppendJsonBool(json, "loupe", g_loupe);

    json += ",\"status\":{\"tone\":\"";
    json += ToneName(g_status.tone);
    json += "\",\"title\":";
    AppendJsonString(json, g_status.title);
    json += ",\"detail\":";
    AppendJsonString(json, g_status.detail);
    json += ",\"notice\":";
    AppendJsonString(json, g_pace.notice);
    json += ",\"chips\":[";
    for (size_t index = 0; index < g_status.chips.size(); ++index) {
        if (index > 0) {
            json += ',';
        }
        AppendJsonString(json, g_status.chips[index]);
    }
    json += "]}";
    AppendJsonBool(json, "tipsReady", g_tips.ready);
    AppendJsonBool(json, "tipsOpen", g_tips.open);
    json += ",\"tips\":[";
    for (size_t index = 0; index < g_tips.current.size(); ++index) {
        const vcam::Tip& tip = g_tips.current[index];
        if (index > 0) {
            json += ',';
        }
        json += "{\"id\":";
        AppendJsonString(json, Wide(tip.id));
        json += ",\"title\":";
        AppendJsonString(json, tip.title);
        json += ",\"why\":";
        AppendJsonString(json, tip.why);
        json += ",\"action\":";
        AppendJsonString(json, tip.actionLabel);
        AppendJsonBool(json, "done", tip.id == g_tips.doneId);
        json += '}';
    }
    json += ']';

    json += ",\"devices\":[";
    for (size_t index = 0; index < g_devices.size(); ++index) {
        const Listed& device = g_devices[index];
        if (index > 0) {
            json += ',';
        }
        json += "{\"key\":";
        AppendJsonString(json, Wide(DeviceKey(device)));
        json += ",\"kind\":\"";
        json += device.kind == Kind::Usb ? "usb" : "wifi";
        json += "\",\"name\":";
        AppendJsonString(json, device.name);
        json += ",\"detail\":";
        AppendJsonString(json, device.detail);
        json += '}';
    }
    json += ']';

    json += ",\"checks\":[";
    const std::vector<Check> checks = BuildChecks();
    for (size_t index = 0; index < checks.size(); ++index) {
        if (index > 0) {
            json += ',';
        }
        json += "{\"state\":\"";
        json += CheckName(checks[index].state);
        json += "\",\"pc\":";
        json += checks[index].pc ? "true" : "false";
        json += ",\"title\":";
        AppendJsonString(json, checks[index].title);
        json += ",\"detail\":";
        AppendJsonString(json, checks[index].detail);
        if (checks[index].action != nullptr) {
            json += ",\"action\":\"";
            json += checks[index].action;
            json += '"';
        }
        json += '}';
    }
    json += ']';

    json += ",\"update\":{\"current\":";
    AppendJsonString(json, Wide(vcam::CurrentVersion()));
    AppendJsonBool(json, "available", g_update.available);
    AppendJsonBool(json, "busy", g_update.busy);
    json += ",\"version\":";
    AppendJsonString(json, Wide(g_update.tag));
    json += ",\"detail\":";
    AppendJsonString(json, g_update.detail);
    json += "}";
    json += "}";
    return json;
}

void PushState() {
    if (g_shell && g_shellReady) {
        g_shell->PostWebMessageAsJson(Wide(BuildState()).c_str());
    }
}

void SetStatus(Tone tone, std::wstring title, std::wstring detail, std::vector<std::wstring> chips = {}) {
    g_status.tone = tone;
    g_status.title = std::move(title);
    g_status.detail = std::move(detail);
    g_status.chips = std::move(chips);
    if (tone != Tone::Live && !g_status.detail.empty()) {
        g_previewSurface.SetPlaceholder(g_status.detail);
    }
    PushState();
}

void ShowIdleStatus() {
    if (Connected()) {
        return;
    }
    if (g_userDisconnected && FindDevice(g_selectedKey) != nullptr) {
        SetStatus(Tone::Idle, L"Disconnected",
                  L"This iPhone is still here. Click Connect when you want the picture back.");
        return;
    }
    if (g_phoneLeft) {
        SetStatus(Tone::Idle, L"The iPhone disconnected", g_phoneLeftDetail);
        return;
    }
    if (!g_connectError.empty()) {
        SetStatus(Tone::Error, L"Couldn\u2019t connect", g_connectError);
    } else if (!g_devices.empty()) {
        SetStatus(Tone::Ready, L"Ready to connect", L"Pick your iPhone, then Connect.");
    } else if (!g_usbError.empty()) {
        SetStatus(Tone::Warn, L"No phone found yet", g_usbError);
    } else {
        SetStatus(Tone::Idle, L"Looking for your iPhone", L"Plug in a data cable and tap Trust, or open VCamdroid on the same Wi-Fi.");
    }
}

std::vector<std::wstring> SessionChips(int battery, int width, int height, int fps) {
    std::vector<std::wstring> chips;
    if (!g_connectedName.empty()) {
        chips.push_back(g_connectedName);
    }
    if (battery >= 0) {
        chips.push_back(std::to_wstring(battery) + L"% battery");
    }
    if (width > 0 && height > 0) {
        std::wstring mode = std::to_wstring(width) + L"\u00d7" + std::to_wstring(height);
        if (fps > 0) {
            mode += L" \u00b7 " + std::to_wstring(fps) + L" fps";
        }
        chips.push_back(std::move(mode));
    }
    return chips;
}

void RoundCorners(HWND window, int width, int height, int radius) {
    if (window == nullptr || width <= 0 || height <= 0) {
        return;
    }
    const int diameter = radius * 2;
    SetWindowRgn(window, CreateRoundRectRgn(0, 0, width + 1, height + 1, diameter, diameter), TRUE);
}

void PlacePane(HWND pane, const RECT& bounds) {
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    RECT current{};
    GetWindowRect(pane, &current);
    const bool resized = current.right - current.left != width || current.bottom - current.top != height;
    MoveWindow(pane, bounds.left, bounds.top, width, height, TRUE);
    if (resized) {
        RoundCorners(pane, width, height, Scale(g_window, kPaneRadius));
    }
    if (!IsWindowVisible(pane)) {
        ShowWindow(pane, SW_SHOWNA);
    }
}

void Layout() {
    if (!g_window || IsIconic(g_window)) {
        return;
    }
    RECT client{};
    GetClientRect(g_window, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    const int bar = (std::min)(Scale(g_window, kToolbarHeight), height);
    const bool session = Connected() && !g_sheetOpen && !g_guideOpen;
    const int tray = (session && g_tips.open) ? Scale(g_window, kTipsTrayHeight) : 0;

    if (g_shellController) {
        RECT shell = client;
        if (session) {
            shell.bottom = bar + tray;
        }
        g_shellController->put_Bounds(shell);
        g_shellController->put_IsVisible(TRUE);
    }

    const int margin = Scale(g_window, 12);
    const RECT content{margin, bar + tray + margin, width - margin, height - margin};
    const int innerW = content.right - content.left;
    const int innerH = content.bottom - content.top;
    if (!session || innerW <= 0 || innerH <= 0) {
        auto hide = [](HWND pane) {
            if (pane == nullptr) {
                return;
            }
            ShowWindow(pane, SW_HIDE);
            MoveWindow(pane, 0, 0, 0, 0, FALSE);
        };
        hide(g_preview);
        hide(g_splitter);
        hide(g_deskHost);
        if (g_deskController) {
            g_deskController->put_IsVisible(FALSE);
        }
        return;
    }

    RECT preview = content;
    RECT desk = content;
    RECT split = content;
    g_stacked = width < Scale(g_window, 900);
    g_innerW = innerW;
    g_innerH = innerH;
    const int hit = Scale(g_window, kSplitterHit);
    // Clamp only for this frame so a small window does not overwrite the saved preference.
    const double ratio = ClampDeskRatio(DeskRatio(g_stacked), g_stacked ? innerH : innerW, g_stacked);
    if (!g_stacked) {
        const int deskW = (std::max)(1, static_cast<int>(innerW * ratio));
        preview.right = content.right - deskW - hit;
        split.left = preview.right;
        split.right = preview.right + hit;
        desk.left = split.right;
    } else {
        const int deskH = (std::max)(1, static_cast<int>(innerH * ratio));
        preview.bottom = content.bottom - deskH - hit;
        split.top = preview.bottom;
        split.bottom = preview.bottom + hit;
        desk.top = split.bottom;
    }
    if (g_preview) {
        PlacePane(g_preview, preview);
    }
    if (g_splitter) {
        MoveWindow(g_splitter, split.left, split.top, split.right - split.left, split.bottom - split.top, TRUE);
        if (!IsWindowVisible(g_splitter)) {
            ShowWindow(g_splitter, SW_SHOWNA);
        }
        InvalidateRect(g_splitter, nullptr, TRUE);
    }
    if (g_deskHost) {
        PlacePane(g_deskHost, desk);
        if (g_deskController) {
            const RECT inside{0, 0, desk.right - desk.left, desk.bottom - desk.top};
            g_deskController->put_Bounds(inside);
            g_deskController->put_IsVisible(TRUE);
        }
    }
}

void NavigateDesk(const std::wstring& url) {
    g_deskUrl = url;
    KillTimer(g_window, kDeskRetryTimer);
    if (g_desk) {
        g_desk->Navigate(DeskTarget().c_str());
    }
}

void StartScan() {
    if (!g_window || g_scanning.exchange(true)) {
        return;
    }
    if (g_scanThread.joinable()) {
        g_scanThread.join();
    }
    const HWND target = g_window;
    g_scanThread = std::thread([target] {
        auto* result = new ScanResult();
        const usbmux::ListResult usb = usbmux::listDevices();
        for (const usbmux::Device& device : usb.devices) {
            Listed row;
            row.kind = Kind::Usb;
            row.usbId = device.id;
            row.serial = device.serial;
            row.name = device.name.empty() ? L"iPhone" : Wide(device.name);
            const std::string suffix = device.serial.size() > 4 ? device.serial.substr(device.serial.size() - 4) : device.serial;
            row.detail = suffix.empty() ? L"USB" : L"USB \u00b7 \u2026" + Wide(suffix);
            result->devices.push_back(std::move(row));
        }
        result->usbError = Wide(usb.error);
        for (const mdns::Service& service : mdns::browse(1000)) {
            Listed row;
            row.kind = Kind::Wifi;
            row.ipv4 = service.ipv4;
            row.port = service.port > 0 ? service.port : 4747;
            row.name = service.name.empty() ? L"iPhone" : Wide(service.name);
            row.detail = L"Wi-Fi \u00b7 " + Wide(service.ipv4);
            result->devices.push_back(std::move(row));
        }
        if (!PostMessageW(target, kScanDoneMessage, 0, reinterpret_cast<LPARAM>(result))) {
            delete result;
        }
    });
    PushState();
}

void StartPoll() {
    if (!g_window || !Connected() || g_polling.exchange(true)) {
        return;
    }
    if (g_pollThread.joinable()) {
        g_pollThread.join();
    }
    const HWND target = g_window;
    const std::string host = g_connectedHost;
    const int port = g_connectedPort;
    g_pollThread = std::thread([target, host, port] {
        auto* poll = new SessionPoll();
        poll->host = host;
        poll->port = port;
        poll->startedMs = GetTickCount64();
        poll->ok = HttpGet(host, port, "/v1/session", poll->body);
        if (poll->ok) {
            ReportLink(host, port, poll->body);
        }
        poll->finishedMs = GetTickCount64();
        if (!PostMessageW(target, kPollDoneMessage, 0, reinterpret_cast<LPARAM>(poll))) {
            delete poll;
        }
    });
}

void StopPicture();

// Runs this exe again elevated with --install-camera, then reports its exit code.
void StartCameraInstall() {
    if (!g_window || g_installing) {
        return;
    }
    if (g_installThread.joinable()) {
        g_installThread.join();
    }
    wchar_t exe[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, exe, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        g_installNote = L"VCamPanel could not find its own folder. Move it to a shorter path and try again.";
        PushState();
        return;
    }
    g_installing = true;
    g_installNote.clear();
    PushState();
    const HWND target = g_window;
    const std::wstring path(exe);
    g_installThread = std::thread([target, path] {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        SHELLEXECUTEINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
        info.hwnd = target;
        info.lpVerb = L"runas";
        info.lpFile = path.c_str();
        info.lpParameters = kInstallSwitch;
        info.nShow = SW_HIDE;
        HRESULT result = S_OK;
        if (!ShellExecuteExW(&info) || info.hProcess == nullptr) {
            result = HRESULT_FROM_WIN32(GetLastError());
        } else {
            WaitForSingleObject(info.hProcess, INFINITE);
            DWORD code = 0;
            result = GetExitCodeProcess(info.hProcess, &code) ? static_cast<HRESULT>(code) : HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(info.hProcess);
        }
        if (SUCCEEDED(com)) {
            CoUninitialize();
        }
        PostMessageW(target, kInstallDoneMessage, static_cast<WPARAM>(static_cast<DWORD>(result)), 0);
    });
}

void FinishCameraInstall(HRESULT result) {
    g_installing = false;
    if (SUCCEEDED(result)) {
        g_installNote.clear();
        vcam::Pipeline::Shared().Refresh();
    } else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        g_installNote = L"The Windows prompt was declined. Click Install camera to try again.";
    } else {
        wchar_t code[16] = {};
        swprintf_s(code, L"%08X", static_cast<unsigned>(result));
        g_installNote = std::wstring(L"The camera install did not finish (error ") + code +
            L"). Click Install camera to try again, and approve the Windows prompt.";
    }
    PushState();
}

bool RecentlyChecked() {
    const std::wstring text = LoadSetting(L"panel-update-check.txt");
    if (text.empty()) {
        return false;
    }
    const long long last = _wtoll(text.c_str());
    const long long now = static_cast<long long>(time(nullptr));
    return last > 0 && now >= last && (now - last) < 15;
}

void StartUpdateCheck(bool manual) {
    if (!g_window || g_update.busy) {
        return;
    }
    if (!manual && (g_update.checked || RecentlyChecked())) {
        return;
    }
    if (!manual && vcam::IsDevBuild()) {
        g_update.detail = L"This is an unreleased build. Click Check for updates to look on GitHub.";
        PushState();
        return;
    }
    g_update.busy = true;
    g_update.detail = L"Checking GitHub for a newer companion.";
    PushState();
    if (g_updateThread.joinable()) {
        g_updateThread.join();
    }
    const HWND target = g_window;
    g_updateThread = std::thread([target] {
        auto* job = new UpdateJob();
        const vcam::UpdateOffer offer = vcam::CheckLatest();
        job->ok = offer.ok;
        job->available = offer.available;
        job->tag = offer.tag;
        job->url = offer.url;
        job->detail = offer.detail;
        if (!PostMessageW(target, kUpdateDoneMessage, 0, reinterpret_cast<LPARAM>(job))) {
            delete job;
        }
    });
}

void StartApplyUpdate() {
    if (!g_window || g_update.busy) {
        return;
    }
    if (g_update.url.empty()) {
        StartUpdateCheck(true);
        return;
    }
    g_update.busy = true;
    g_update.detail = L"Downloading the new companion. The picture will stop, then this window restarts.";
    PushState();
    if (g_updateThread.joinable()) {
        g_updateThread.join();
    }
    const HWND target = g_window;
    const std::string url = g_update.url;
    g_updateThread = std::thread([target, url] {
        auto* job = new UpdateJob();
        job->apply = true;
        const vcam::UpdateStage stage = vcam::PrepareUpdate(url);
        job->ok = stage.ok;
        job->detail = stage.detail;
        job->stage = stage.folder;
        if (!PostMessageW(target, kUpdateDoneMessage, 0, reinterpret_cast<LPARAM>(job))) {
            delete job;
        }
    });
}

void FinishUpdateJob(std::unique_ptr<UpdateJob> job) {
    if (!job) {
        return;
    }
    g_update.busy = false;
    if (job->apply) {
        if (!job->ok) {
            g_update.detail = job->detail.empty()
                ? L"The update did not finish. The folder may not be writable, or GitHub may be blocked."
                : job->detail;
            PushState();
            return;
        }
        if (Connected()) {
            StopPicture();
        } else {
            vcam::Pipeline::Shared().Stop();
        }
        if (!vcam::LaunchUpdater(job->stage)) {
            g_update.detail = L"The updater could not start. Move VCamPanel to a folder you can write, then try again.";
            PushState();
            return;
        }
        DestroyWindow(g_window);
        return;
    }
    g_update.checked = true;
    g_update.ok = job->ok;
    g_update.available = job->available;
    g_update.tag = job->tag;
    g_update.url = job->url;
    g_update.detail = job->detail;
    if (job->ok) {
        wchar_t stamp[32] = {};
        swprintf_s(stamp, L"%lld", static_cast<long long>(time(nullptr)));
        SaveSetting(L"panel-update-check.txt", stamp);
    }
    PushState();
}

void ResetPace() {
    g_pace = {};
    ResetTips();
}

void ConnectTo(const Listed& target) {
    KillTimer(g_window, kStatusTimer);
    StopObsTally();
    vcam::Pipeline::Shared().Stop();
    usbmux::stop();
    ResetPace();
    g_userDisconnected = false;
    g_phoneLeft = false;
    g_sessionMisses = 0;
    g_reportedLink.clear();
    g_usbConnected = false;
    g_connectedHost.clear();
    g_connectError.clear();
    g_lastTarget = target;
    g_haveLastTarget = true;

    if (target.kind == Kind::Usb) {
        std::string error;
        if (!usbmux::start(target.port, target.usbId, error)) {
            g_connectError = Wide(error);
            Layout();
            ShowIdleStatus();
            return;
        }
        g_usbConnected = true;
        g_connectedHost = "127.0.0.1";
    } else {
        if (target.ipv4.empty()) {
            g_connectError = L"Type the Wi-Fi address shown on the phone.";
            Layout();
            ShowIdleStatus();
            return;
        }
        g_connectedHost = target.ipv4;
        g_address = Wide(target.ipv4) + L":" + std::to_wstring(target.port);
        SaveSetting(L"panel-address.txt", g_address);
    }
    g_connectedPort = target.port;
    g_connectedName = target.name;
    g_obsAddress = Wide(g_connectedHost) + L":" + std::to_wstring(g_connectedPort);
    KillTimer(g_window, kScanTimer);
    NavigateDesk(L"http://" + g_obsAddress + L"/panel?companion=1");
    vcam::Pipeline::Shared().Start(g_connectedHost, g_connectedPort);
    Layout();

    const std::wstring warning = vcam::Pipeline::Shared().Warning();
    SetStatus(
        Tone::Busy,
        g_usbConnected ? L"Connected over USB" : L"Connected over Wi-Fi",
        vcam::Pipeline::Shared().Available() || warning.empty() ? std::wstring(kPhoneCue) : warning,
        SessionChips(-1, 0, 0, 0));
    SetTimer(g_window, kStatusTimer, kStatusIntervalMs, nullptr);
    StartPoll();
    StartObsTally(g_connectedHost, g_connectedPort, g_obsPassword);
}

void ConnectFromShell(const std::string& key, const std::wstring& typed) {
    std::string host;
    int port = 4747;
    const bool haveTyped = ParseHostPort(typed, host, port);
    if (!key.empty()) {
        if (const Listed* device = FindDevice(key)) {
            Listed target = *device;
            if (target.kind == Kind::Usb) {
                target.port = haveTyped ? port : 4747;
            }
            ConnectTo(target);
            return;
        }
    }
    if (!haveTyped) {
        g_connectError = Trim(typed).empty()
            ? L"Pick a phone in the list, or type the Wi-Fi address shown on the phone."
            : L"That address doesn\u2019t look right. Use the one shown on the phone, like 192.168.1.20:4747.";
        ShowIdleStatus();
        return;
    }
    Listed target;
    target.kind = Kind::Wifi;
    target.ipv4 = host;
    target.port = port;
    target.name = Wide(host);
    for (const Listed& device : g_devices) {
        if (device.kind == Kind::Wifi && device.ipv4 == host) {
            target.name = device.name;
            break;
        }
    }
    ConnectTo(target);
}

Listed UsbTarget(const Listed& device) {
    Listed target = device;
    std::string host;
    int port = 4747;
    target.port = ParseHostPort(g_address, host, port) ? port : 4747;
    return target;
}

bool AutoConnect() {
    if (g_userDisconnected) {
        return false;
    }
    for (const Listed& device : g_devices) {
        if (device.kind == Kind::Usb) {
            ConnectTo(UsbTarget(device));
            return true;
        }
    }
    std::string host;
    int port = 4747;
    if (!ParseHostPort(g_address, host, port)) {
        return false;
    }
    for (const Listed& device : g_devices) {
        if (device.kind == Kind::Wifi && device.ipv4 == host) {
            Listed target = device;
            target.port = port;
            ConnectTo(target);
            return true;
        }
    }
    return false;
}

void StopPicture() {
    KillTimer(g_window, kStatusTimer);
    KillTimer(g_window, kDeskRetryTimer);
    StopObsTally();
    vcam::Pipeline::Shared().Stop();
    usbmux::stop();
    ResetPace();
    g_sessionMisses = 0;
    g_reportedLink.clear();
    g_usbConnected = false;
    g_connectedHost.clear();
    g_connectedName.clear();
    g_connectError.clear();
    g_deskUrl.clear();
    g_sheetOpen = false;
    g_guideOpen = false;
    if (g_desk) {
        g_desk->Navigate(L"about:blank");
    }
    Layout();
    SetTimer(g_window, kScanTimer, kScanIntervalMs, nullptr);
}

void Disconnect() {
    const bool stillHere = FindDevice(g_selectedKey) != nullptr;
    StopPicture();
    g_userDisconnected = true;
    g_phoneLeft = false;
    if (stillHere) {
        SetStatus(Tone::Idle, L"Disconnected",
                  L"This iPhone is still here. Click Connect when you want the picture back.");
    } else {
        SetStatus(Tone::Idle, L"Looking for your iPhone",
                  L"Plug in a data cable and tap Trust, or open VCamdroid on the same Wi-Fi.");
    }
    StartScan();
}

void PhoneWentAway(bool cable) {
    StopPicture();
    g_userDisconnected = false;
    g_phoneLeft = true;
    g_phoneLeftDetail = cable ? L"The cable came out. Plug it in to connect again."
                              : L"The phone left the Wi-Fi. It will show up here when it is back.";
    SetStatus(Tone::Idle, L"The iPhone disconnected", g_phoneLeftDetail);
    StartScan();
}

bool PhoneStillListed() {
    if (g_usbConnected) {
        for (const Listed& device : g_devices) {
            if (device.kind != Kind::Usb || device.usbId != g_lastTarget.usbId) {
                continue;
            }
            if (!g_lastTarget.serial.empty() && !device.serial.empty() && device.serial != g_lastTarget.serial) {
                continue;
            }
            return true;
        }
        return false;
    }
    for (const Listed& device : g_devices) {
        if (device.kind == Kind::Wifi && device.ipv4 == g_connectedHost && device.missed < kWifiGraceScans) {
            return true;
        }
    }
    return false;
}

void ApplyScan(std::unique_ptr<ScanResult> result) {
    g_scanning = false;
    std::vector<Listed> merged = std::move(result->devices);
    std::string newUsbKey;
    for (const Listed& device : merged) {
        if (device.kind == Kind::Usb && newUsbKey.empty() && FindDevice(DeviceKey(device)) == nullptr) {
            newUsbKey = DeviceKey(device);
        }
    }
    for (const Listed& old : g_devices) {
        if (old.kind != Kind::Wifi || old.missed + 1 >= kWifiGraceScans) {
            continue;
        }
        const std::string key = DeviceKey(old);
        const bool present = std::any_of(merged.begin(), merged.end(),
            [&key](const Listed& device) { return DeviceKey(device) == key; });
        if (!present) {
            Listed kept = old;
            ++kept.missed;
            merged.push_back(std::move(kept));
        }
    }
    g_devices = std::move(merged);
    g_usbError = std::move(result->usbError);
    const std::string heldSelection = g_selectedKey;
    if (FindDevice(g_selectedKey) == nullptr) {
        const bool lost = !g_selectedKey.empty();
        g_selectedKey.clear();
        if (lost) {
            g_userClearedSelection = false;
        }
        if (!g_userClearedSelection) {
            const Listed* onlyUsb = nullptr;
            int usbCount = 0;
            for (const Listed& device : g_devices) {
                if (device.kind == Kind::Usb) {
                    onlyUsb = &device;
                    ++usbCount;
                }
            }
            if (usbCount == 1) {
                g_selectedKey = DeviceKey(*onlyUsb);
            } else if (g_devices.size() == 1) {
                g_selectedKey = DeviceKey(g_devices.front());
            }
        }
    }

    if (Connected()) {
        if (PhoneStillListed()) {
            PushState();
            return;
        }
        const bool cable = g_usbConnected;
        if (!heldSelection.empty()) {
            g_selectedKey = heldSelection;
        }
        PhoneWentAway(cable);
        if (newUsbKey.empty()) {
            return;
        }
    }
    const bool first = g_firstScan;
    g_firstScan = false;
    if (first && AutoConnect()) {
        return;
    }
    if (!first && !newUsbKey.empty()) {
        if (const Listed* device = FindDevice(newUsbKey)) {
            ConnectTo(UsbTarget(*device));
            return;
        }
    }
    ShowIdleStatus();
}

int UpdatePace(const SessionPoll& poll, const vcam::LinkProgress& progress) {
    if (!poll.ok) {
        return -1;
    }
    const unsigned long long now = GetTickCount64();
    const unsigned long long clockMs = JsonU64(poll.body, "clockMs");
    if (clockMs != 0 && poll.finishedMs >= poll.startedMs) {
        const unsigned long long mid = poll.startedMs + (poll.finishedMs - poll.startedMs) / 2;
        g_pace.offset = static_cast<long long>(clockMs) - static_cast<long long>(mid);
        g_pace.haveOffset = true;
    }

    int delay = -1;
    const bool picture = progress.lastCaptureMs != 0 && progress.lastCaptureMs != ~0ull && progress.lastDecodedMs != 0;
    if (g_pace.haveOffset && picture) {
        const long long capturedAt = static_cast<long long>(progress.lastCaptureMs) - g_pace.offset;
        const long long late = static_cast<long long>(progress.lastDecodedMs) - capturedAt;
        if (late >= 0 && late < 10000) {
            delay = static_cast<int>(late);
        }
    }
    if (delay >= 0 && g_pace.lastDelay >= 0 && delay > g_pace.lastDelay + 30) {
        if (g_pace.climbSince == 0) {
            g_pace.climbSince = now;
        }
    } else {
        g_pace.climbSince = 0;
    }
    g_pace.lastDelay = delay;
    const bool climbing = g_pace.climbSince != 0 && now - g_pace.climbSince >= 2000;

    if (g_pace.fpsSampleMs != 0 && now - g_pace.fpsSampleMs >= 800 && progress.decodedFrames >= g_pace.decodedFrames) {
        const double seconds = static_cast<double>(now - g_pace.fpsSampleMs) / 1000.0;
        g_pace.fps = static_cast<double>(progress.decodedFrames - g_pace.decodedFrames) / seconds;
        g_pace.decodedFrames = progress.decodedFrames;
        g_pace.fpsSampleMs = now;
    } else if (g_pace.fpsSampleMs == 0) {
        g_pace.fpsSampleMs = now;
        g_pace.decodedFrames = progress.decodedFrames;
    }

    const int drops = JsonInt(poll.body, "droppedFrames", 0);
    const bool dropsIncreased = drops > g_pace.drops;
    g_pace.drops = drops;
    const bool repeatsClimbing = g_pace.haveRepeats && progress.repeats > g_pace.repeats;
    g_pace.haveRepeats = true;
    g_pace.repeats = progress.repeats;
    g_pace.repeatsClimbing = repeatsClimbing;
    const std::string thermal = JsonString(poll.body, "thermal");

    std::wstring condition;
    if (delay > 150 || climbing) {
        condition = L"Running behind";
    } else if (progress.state == vcam::LinkState::Live && g_pace.fps > 0 && g_pace.fps < 27) {
        condition = L"Dropping frames";
    } else if (dropsIncreased) {
        condition = L"USB couldn\u2019t keep up";
    } else if (thermal == "serious" || thermal == "critical") {
        condition = L"Phone is hot, quality may drop";
    } else if (repeatsClimbing) {
        condition = L"The recording app is getting repeated frames";
    }

    if (!condition.empty()) {
        g_pace.notice = std::move(condition);
        g_pace.healthySince = 0;
    } else if (!g_pace.notice.empty()) {
        if (g_pace.healthySince == 0) {
            g_pace.healthySince = now;
        } else if (now - g_pace.healthySince >= 2000) {
            g_pace.notice.clear();
            g_pace.healthySince = 0;
        }
    }
    return delay;
}

bool DropNotice() {
    const std::wstring& notice = g_pace.notice;
    return notice == L"Dropping frames" || notice == L"USB couldn\u2019t keep up" ||
           notice == L"Phone is hot, quality may drop" ||
           notice == L"The recording app is getting repeated frames" ||
           notice == L"Running behind";
}

void UpdateTips(const SessionPoll& poll, int delayMs) {
    const unsigned long long now = GetTickCount64();
    const int phoneDrops = JsonInt(poll.body, "droppedFrames", 0);
    const int captureDrops = JsonInt(poll.body, "captureDrops", 0);
    if (phoneDrops > g_tips.lastPhoneDrops) {
        g_tips.phoneRecent = now;
    }
    if (captureDrops > g_tips.lastCaptureDrops) {
        g_tips.captureRecent = now;
    }
    g_tips.lastPhoneDrops = phoneDrops;
    g_tips.lastCaptureDrops = captureDrops;
    vcam::AdviceInput input;
    input.fps = g_pace.fps;
    input.phoneDrops = g_tips.phoneRecent != 0 && now - g_tips.phoneRecent < 8000 ? 1 : 0;
    input.captureDrops = g_tips.captureRecent != 0 && now - g_tips.captureRecent < 8000 ? 1 : 0;
    input.repeatsClimbing = g_pace.repeatsClimbing;
    input.delayMs = delayMs;
    input.cpuPercent = SampleCpu();
    input.usb = g_usbConnected;
    input.thermal = JsonString(poll.body, "thermal");
    input.portrait = JsonBool(poll.body, "portrait");
    input.manualExposure = JsonBool(poll.body, "manualExposure");
    input.shutter = JsonNumber(poll.body, "shutter", 0);
    input.torch = JsonBool(poll.body, "torch");
    input.lowPower = JsonBool(poll.body, "lowPower");
    input.charging = JsonBool(poll.body, "charging");
    input.battery = JsonInt(poll.body, "battery", -1);

    std::vector<vcam::Tip> tips = vcam::Advise(input);
    tips.erase(std::remove_if(tips.begin(), tips.end(),
                   [](const vcam::Tip& tip) { return TipDismissed(tip.id); }),
        tips.end());

    if (!g_tips.doneId.empty() && g_tips.doneAt != 0 && now - g_tips.doneAt >= 10000) {
        const bool still = !tips.empty() || DropNotice();
        if (still) {
            g_tips.dismissed.push_back(g_tips.doneId);
            tips.erase(std::remove_if(tips.begin(), tips.end(),
                           [](const vcam::Tip& tip) { return tip.id == g_tips.doneId; }),
                tips.end());
        }
        g_tips.doneId.clear();
        g_tips.doneAt = 0;
    }

    const bool troubled = !tips.empty() || DropNotice();
    if (troubled) {
        if (g_tips.problemSince == 0) {
            g_tips.problemSince = now;
        }
        g_tips.healthySince = 0;
        if (!tips.empty()) {
            g_tips.current = std::move(tips);
        }
        g_tips.ready = now - g_tips.problemSince >= 5000 && !g_tips.current.empty();
        return;
    }

    g_tips.problemSince = 0;
    if (g_tips.healthySince == 0) {
        g_tips.healthySince = now;
    }
    if (now - g_tips.healthySince >= 10000) {
        const bool wasOpen = g_tips.open;
        g_tips.ready = false;
        g_tips.open = false;
        g_tips.current.clear();
        g_tips.doneId.clear();
        g_tips.doneAt = 0;
        if (wasOpen) {
            Layout();
        }
    }
}

void SetTipsOpen(bool open) {
    g_tips.open = open && g_tips.ready && !g_tips.current.empty();
    Layout();
    PushState();
}

void StartTipAction(const std::string& id, const std::string& path) {
    if (path.empty() || !Connected()) {
        return;
    }
    g_pendingTipId = id;
    if (g_tipThread.joinable()) {
        g_tipThread.join();
    }
    const HWND target = g_window;
    const std::string host = g_connectedHost;
    const int port = g_connectedPort;
    g_tipThread = std::thread([target, host, port, path] {
        HttpPut(host, port, path);
        if (target) {
            PostMessageW(target, kTipDoneMessage, 0, 0);
        }
    });
}

void ApplySession(const SessionPoll& poll) {
    if (!Connected() || poll.host != g_connectedHost || poll.port != g_connectedPort) {
        return;
    }
    const vcam::LinkProgress progress = vcam::Pipeline::Shared().Progress();
    const vcam::LinkState link = progress.state;
    const std::wstring transport = g_usbConnected ? L"USB" : L"Wi-Fi";
    const bool phoneSending = JsonBool(poll.body, "streaming") || JsonBool(poll.body, "panelClient");
    if (link == vcam::LinkState::Interrupted) {
        if (g_tips.open) {
            g_tips.open = false;
            g_tips.ready = false;
            Layout();
        }
        SetStatus(Tone::Error, L"The video connection closed",
            L"Another app may have taken the phone\u2019s video. Click Connect to try again.",
            SessionChips(-1, 0, 0, 0));
        return;
    }
    if (!poll.ok) {
        if (!g_usbConnected && ++g_sessionMisses >= 3) {
            PhoneWentAway(false);
            return;
        }
        const std::wstring detail = progress.closedWithoutVideo && g_usbConnected ? std::wstring(kUsbAppClosed) : std::wstring(kPhoneCue);
        SetStatus(Tone::Busy, L"Waiting for the iPhone", detail, SessionChips(-1, 0, 0, 0));
        return;
    }
    g_sessionMisses = 0;
    const int battery = JsonInt(poll.body, "battery", -1);
    const int width = JsonInt(poll.body, "width", 0);
    const int height = JsonInt(poll.body, "height", 0);
    const int fps = JsonInt(poll.body, "fps", 0);
    const int delayMs = UpdatePace(poll, progress);
    const wchar_t* installNotice = L"Open Setup and click Install camera so other apps can select VCamdroid.";
    const bool needsInstall = vcam::Pipeline::Shared().Setup().needsInstall;
    if (link == vcam::LinkState::Live && needsInstall &&
        (g_pace.notice.empty() || g_pace.notice == installNotice)) {
        g_pace.notice = installNotice;
        g_pace.healthySince = 0;
    } else if (g_pace.notice == installNotice) {
        g_pace.notice.clear();
        g_pace.healthySince = 0;
    }
    auto chips = SessionChips(battery, width, height, fps);
    const bool ready = link == vcam::LinkState::Live && delayMs >= 0 && delayMs < 150 && g_pace.fps >= 27 &&
                       g_pace.notice.empty();
    if (ready) {
        chips.push_back(L"Ready");
    } else if (delayMs >= 0 && delayMs < 150 && g_pace.notice.empty()) {
        chips.push_back(std::to_wstring(delayMs) + L" ms");
    }

    Tone tone = Tone::Busy;
    std::wstring title = L"Connected over " + transport;
    if (link == vcam::LinkState::Live) {
        tone = Tone::Live;
        title = L"Streaming over " + transport;
    } else if (link == vcam::LinkState::Connecting) {
        title = progress.lastDecodedMs != 0 ? L"Reconnecting" : L"Opening the picture";
    }

    std::wstring detail;
    const std::wstring warning = vcam::Pipeline::Shared().Warning();
    if (tone == Tone::Live && needsInstall) {
        if (g_pace.notice.empty() || g_pace.notice == installNotice) {
            g_pace.notice = installNotice;
            g_pace.healthySince = 0;
        }
        detail = L"Choose VCamdroid as the camera in NVIDIA Broadcast, OBS, Zoom, Teams, Meet, or Discord.";
    } else if (needsInstall) {
        detail = installNotice;
        if (tone == Tone::Live) {
            tone = Tone::Warn;
        }
    } else if (!vcam::Pipeline::Shared().Available() && !warning.empty()) {
        detail = warning;
        if (tone == Tone::Live) {
            tone = Tone::Warn;
        }
    } else if (link == vcam::LinkState::Live) {
        detail = L"Choose VCamdroid as the camera in NVIDIA Broadcast, OBS, Zoom, Teams, Meet, or Discord.";
    } else if (phoneSending && ReceivingRecently(progress) && !DecodedRecently(progress)) {
        detail = kCannotDecode;
        tone = Tone::Warn;
    } else if (progress.closedWithoutVideo && g_usbConnected) {
        detail = kUsbAppClosed;
        tone = Tone::Warn;
    } else {
        detail = kPhoneCue;
    }
    SetStatus(tone, std::move(title), std::move(detail), std::move(chips));
    if (link == vcam::LinkState::Live) {
        UpdateTips(poll, delayMs);
        PushState();
    } else if (g_tips.ready || g_tips.open) {
        g_tips.ready = false;
        g_tips.open = false;
        Layout();
        PushState();
    }
}

void SetOnTop(bool onTop) {
    g_onTop = onTop;
    SetWindowPos(g_window, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SaveSetting(L"panel-on-top.txt", onTop ? L"1" : L"0");
    PushState();
}

void SetSheet(bool open) {
    g_sheetOpen = open;
    if (open) {
        g_guideOpen = false;
    }
    Layout();
    PushState();
}

void SetGuide(bool open) {
    g_guideOpen = open;
    if (open) {
        g_sheetOpen = false;
    }
    Layout();
    PushState();
}

void HandleShellMessage(const std::string& json) {
    const std::string type = JsonString(json, "type");
    if (type == "ready") {
        g_shellReady = true;
        Layout();
        PushState();
        StartUpdateCheck(false);
    } else if (type == "refresh") {
        g_connectError.clear();
        StartScan();
        ShowIdleStatus();
    } else if (type == "select") {
        const std::string key = JsonString(json, "key");
        if (key.empty() || FindDevice(key) == nullptr) {
            g_selectedKey.clear();
            g_userClearedSelection = true;
        } else {
            g_selectedKey = key;
            g_userClearedSelection = false;
        }
        PushState();
    } else if (type == "connect") {
        const std::string key = JsonString(json, "key");
        if (key.empty()) {
            g_selectedKey.clear();
        } else if (FindDevice(key) != nullptr) {
            g_selectedKey = key;
            g_userClearedSelection = false;
        }
        ConnectFromShell(g_selectedKey, Wide(JsonString(json, "address")));
    } else if (type == "reconnect") {
        if (g_haveLastTarget) {
            ConnectTo(g_lastTarget);
        }
    } else if (type == "disconnect") {
        Disconnect();
    } else if (type == "copy") {
        CopyText(g_obsAddress);
    } else if (type == "onTop") {
        SetOnTop(JsonBool(json, "value"));
    } else if (type == "sheet") {
        SetSheet(JsonBool(json, "open"));
    } else if (type == "guide") {
        SetGuide(JsonBool(json, "open"));
    } else if (type == "installCamera") {
        StartCameraInstall();
    } else if (type == "obsPassword") {
        g_obsPassword = Wide(JsonString(json, "value"));
        SaveSetting(L"panel-obs-password.txt", g_obsPassword);
        if (Connected()) {
            StartObsTally(g_connectedHost, g_connectedPort, g_obsPassword);
        }
        PushState();
    } else if (type == "monitor") {
        const std::string tool = JsonString(json, "tool");
        const bool on = JsonBool(json, "value");
        if (tool == "peaking") {
            g_peaking = on;
        } else if (tool == "zebra") {
            g_zebra = on;
        } else if (tool == "loupe") {
            g_loupe = on;
        }
        g_previewSurface.SetChecks(g_peaking, g_zebra, g_loupe);
        PushState();
    } else if (type == "tips") {
        SetTipsOpen(JsonBool(json, "open"));
    } else if (type == "tipAction") {
        const std::string id = JsonString(json, "id");
        for (const vcam::Tip& tip : g_tips.current) {
            if (tip.id == id) {
                StartTipAction(id, tip.actionPath);
                break;
            }
        }
    } else if (type == "tipDismiss") {
        const std::string id = JsonString(json, "id");
        if (!id.empty() && !TipDismissed(id)) {
            g_tips.dismissed.push_back(id);
        }
        g_tips.current.erase(std::remove_if(g_tips.current.begin(), g_tips.current.end(),
                                 [&id](const vcam::Tip& tip) { return tip.id == id; }),
            g_tips.current.end());
        if (g_tips.doneId == id) {
            g_tips.doneId.clear();
            g_tips.doneAt = 0;
        }
        if (g_tips.current.empty()) {
            g_tips.ready = false;
            g_tips.open = false;
            Layout();
        }
        PushState();
    } else if (type == "checkUpdate") {
        StartUpdateCheck(true);
    } else if (type == "applyUpdate") {
        StartApplyUpdate();
    }
}

bool ApplyWindowMaterial(HWND window) {
    const BOOL dark = TRUE;
    DwmSetWindowAttribute(window, static_cast<DWMWINDOWATTRIBUTE>(kDwmDarkMode), &dark, sizeof(dark));
    const int corners = kDwmRoundCorners;
    DwmSetWindowAttribute(window, static_cast<DWMWINDOWATTRIBUTE>(kDwmCornerPreference), &corners, sizeof(corners));
    const int backdrop = kDwmMicaBackdrop;
    if (FAILED(DwmSetWindowAttribute(window, static_cast<DWMWINDOWATTRIBUTE>(kDwmBackdropType), &backdrop, sizeof(backdrop)))) {
        return false;
    }
    const MARGINS extend{-1, -1, -1, -1};
    return SUCCEEDED(DwmExtendFrameIntoClientArea(window, &extend));
}

void ShowWebViewError() {
    if (g_webViewErrorShown) {
        return;
    }
    g_webViewErrorShown = true;
    MessageBoxW(
        g_window,
        L"The WebView2 Runtime is required. Install the Microsoft Edge WebView2 Runtime, then open this window again.",
        L"VCamdroid",
        MB_OK | MB_ICONERROR
    );
}

// Guide links leave the app only for the video and photo credits. Anything else stays blocked.
void OpenGuideLink(const std::wstring& uri) {
    static constexpr const wchar_t* kAllowed[] = {
        L"https://www.youtube.com/",
        L"https://youtu.be/",
        L"https://www.flickr.com/",
        L"https://commons.wikimedia.org/",
        L"https://creativecommons.org/",
    };
    for (const wchar_t* prefix : kAllowed) {
        if (uri.rfind(prefix, 0) == 0) {
            ShellExecuteW(nullptr, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
    }
}

void ConfigureWebView(ICoreWebView2* webview, ICoreWebView2Controller* controller, const COREWEBVIEW2_COLOR& background) {
    ComPtr<ICoreWebView2Controller> owner(controller);
    ComPtr<ICoreWebView2Controller2> controller2;
    if (SUCCEEDED(owner.As(&controller2)) && controller2) {
        controller2->put_DefaultBackgroundColor(background);
    }
    ComPtr<ICoreWebView2Settings> settings;
    if (SUCCEEDED(webview->get_Settings(&settings)) && settings) {
        settings->put_IsStatusBarEnabled(FALSE);
        settings->put_IsZoomControlEnabled(FALSE);
        settings->put_IsBuiltInErrorPageEnabled(FALSE);
#ifdef NDEBUG
        settings->put_AreDevToolsEnabled(FALSE);
#endif
        ComPtr<ICoreWebView2Settings3> settings3;
        if (SUCCEEDED(settings.As(&settings3)) && settings3) {
            settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
        }
    }
    EventRegistrationToken token{};
    webview->add_NewWindowRequested(
        Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                LPWSTR uri = nullptr;
                if (SUCCEEDED(args->get_Uri(&uri)) && uri != nullptr) {
                    OpenGuideLink(uri);
                    CoTaskMemFree(uri);
                }
                args->put_Handled(TRUE);
                return S_OK;
            }
        ).Get(),
        &token
    );
}

std::string GuidePhotoUri(const guide_photos::Photo& photo) {
    std::string uri = "data:image/jpeg;base64,";
    for (int index = 0; index < photo.count; ++index) {
        uri += photo.chunks[index];
    }
    return uri;
}

std::wstring ShellHtml() {
    std::string html;
    html += panel_shell::kHead;
    html += panel_shell::kStyleBase;
    html += panel_shell::kStyleViews;
    html += panel_shell::kGuideStyle;
    html += panel_shell::kBody;
    html += panel_shell::kGuideBody;
    const auto heatAt = html.find("{{heat-panel}}");
    if (heatAt != std::string::npos) {
        html.replace(heatAt, std::string("{{heat-panel}}").size(), panel_shell::kHeatPanel);
    }
    html += panel_shell::kScript;
    for (const guide_photos::Photo& photo : guide_photos::kPhotos) {
        const std::string token = std::string("{{guide:") + photo.name + "}}";
        const auto at = html.find(token);
        if (at != std::string::npos) {
            html.replace(at, token.size(), GuidePhotoUri(photo));
        }
    }
    return Wide(html);
}

void CreateShell(ICoreWebView2Environment* environment) {
    environment->CreateCoreWebView2Controller(
        g_window,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [](HRESULT created, ICoreWebView2Controller* controller) -> HRESULT {
                if (FAILED(created) || controller == nullptr) {
                    ShowWebViewError();
                    return S_OK;
                }
                g_shellController = controller;
                if (FAILED(g_shellController->get_CoreWebView2(&g_shell)) || !g_shell) {
                    ShowWebViewError();
                    return S_OK;
                }
                ConfigureWebView(g_shell.Get(), controller, g_mica ? kClearBg : kOpaqueBg);
                EventRegistrationToken token{};
                g_shell->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                            LPWSTR json = nullptr;
                            if (SUCCEEDED(args->get_WebMessageAsJson(&json)) && json != nullptr) {
                                const std::string message = Narrow(json);
                                CoTaskMemFree(json);
                                HandleShellMessage(message);
                            }
                            return S_OK;
                        }
                    ).Get(),
                    &token
                );
                g_shell->add_NavigationStarting(
                    Callback<ICoreWebView2NavigationStartingEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                            LPWSTR uri = nullptr;
                            std::wstring target;
                            if (SUCCEEDED(args->get_Uri(&uri)) && uri != nullptr) {
                                target = uri;
                                CoTaskMemFree(uri);
                            }
                            const bool local = target.rfind(L"about:", 0) == 0 || target.rfind(L"data:", 0) == 0;
                            if (!g_shellNavigationAllowed && !local) {
                                args->put_Cancel(TRUE);
                            }
                            g_shellNavigationAllowed = false;
                            return S_OK;
                        }
                    ).Get(),
                    &token
                );
                Layout();
                g_shellNavigationAllowed = true;
                g_shell->NavigateToString(ShellHtml().c_str());
                return S_OK;
            }
        ).Get()
    );
}

void CreateDesk(ICoreWebView2Environment* environment) {
    environment->CreateCoreWebView2Controller(
        g_deskHost,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [](HRESULT created, ICoreWebView2Controller* controller) -> HRESULT {
                if (FAILED(created) || controller == nullptr) {
                    ShowWebViewError();
                    return S_OK;
                }
                g_deskController = controller;
                if (FAILED(g_deskController->get_CoreWebView2(&g_desk)) || !g_desk) {
                    ShowWebViewError();
                    return S_OK;
                }
                ConfigureWebView(g_desk.Get(), controller, kOpaqueBg);
                EventRegistrationToken token{};
                g_desk->add_NavigationCompleted(
                    Callback<ICoreWebView2NavigationCompletedEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                            if (!Connected() || g_deskUrl.empty()) {
                                return S_OK;
                            }
                            BOOL ok = FALSE;
                            args->get_IsSuccess(&ok);
                            COREWEBVIEW2_WEB_ERROR_STATUS status = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
                            args->get_WebErrorStatus(&status);
                            if (ok || status == COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED) {
                                return S_OK;
                            }
                            g_desk->NavigateToString(kDeskWaitingHtml);
                            SetTimer(g_window, kDeskRetryTimer, kDeskRetryMs, nullptr);
                            return S_OK;
                        }
                    ).Get(),
                    &token
                );
                g_desk->add_WebMessageReceived(
                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                            LPWSTR text = nullptr;
                            if (FAILED(args->TryGetWebMessageAsString(&text)) || text == nullptr) {
                                return S_OK;
                            }
                            const std::wstring message = text;
                            CoTaskMemFree(text);
                            constexpr wchar_t kPrefix[] = L"desk-layout:";
                            if (message.rfind(kPrefix, 0) != 0) {
                                return S_OK;
                            }
                            const std::wstring layout = message.substr(wcslen(kPrefix));
                            if (!ValidDeskLayout(layout) || layout == g_deskLayout) {
                                return S_OK;
                            }
                            g_deskLayout = layout;
                            SaveSetting(L"desk-layout.json", layout);
                            return S_OK;
                        }
                    ).Get(),
                    &token
                );
                Layout();
                if (Connected() && !g_deskUrl.empty()) {
                    g_desk->Navigate(DeskTarget().c_str());
                }
                return S_OK;
            }
        ).Get()
    );
}

void StartWebView() {
    const std::wstring folder = WebViewFolder();
    const HRESULT requested = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        folder.empty() ? nullptr : folder.c_str(),
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                if (FAILED(result) || environment == nullptr) {
                    ShowWebViewError();
                    return S_OK;
                }
                CreateShell(environment);
                CreateDesk(environment);
                return S_OK;
            }
        ).Get()
    );
    if (FAILED(requested)) {
        ShowWebViewError();
    }
}

LRESULT CALLBACK SplitterProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SETCURSOR:
        SetCursor(LoadCursorW(nullptr, g_stacked ? IDC_SIZENS : IDC_SIZEWE));
        return TRUE;
    case WM_LBUTTONDOWN: {
        POINT cursor{};
        GetCursorPos(&cursor);
        g_splitting = true;
        g_splitOrigin = g_stacked ? cursor.y : cursor.x;
        g_splitRatioOrigin = ClampDeskRatio(DeskRatio(g_stacked), g_stacked ? g_innerH : g_innerW, g_stacked);
        SetCapture(window);
        return 0;
    }
    case WM_MOUSEMOVE:
        if (!g_splitting || (wparam & MK_LBUTTON) == 0) {
            return 0;
        }
        {
            POINT cursor{};
            GetCursorPos(&cursor);
            const int inner = g_stacked ? g_innerH : g_innerW;
            if (inner > 0) {
                const int delta = g_stacked ? (cursor.y - g_splitOrigin) : (cursor.x - g_splitOrigin);
                DeskRatio(g_stacked) = ClampDeskRatio(g_splitRatioOrigin - static_cast<double>(delta) / inner, inner, g_stacked);
                Layout();
            }
        }
        return 0;
    case WM_LBUTTONUP:
    case WM_CAPTURECHANGED:
        if (g_splitting) {
            g_splitting = false;
            if (GetCapture() == window) {
                ReleaseCapture();
            }
            PersistDeskRatio();
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        FillRect(dc, &client, g_background);
        const int midX = (client.left + client.right) / 2;
        const int midY = (client.top + client.bottom) / 2;
        const int thick = (std::max)(2, Scale(g_window, 3));
        const int length = Scale(g_window, 28);
        RECT handle{};
        if (g_stacked) {
            handle = {midX - length / 2, midY - thick / 2, midX + length / 2, midY + (thick + 1) / 2};
        } else {
            handle = {midX - thick / 2, midY - length / 2, midX + (thick + 1) / 2, midY + length / 2};
        }
        HBRUSH brush = CreateSolidBrush(g_splitting ? RGB(217, 160, 74) : RGB(92, 96, 102));
        FillRect(dc, &handle, brush);
        DeleteObject(brush);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    default:
        return DefWindowProcW(window, message, wparam, lparam);
    }
}

HWND CreateSplitter(HWND parent, HINSTANCE instance) {
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = SplitterProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_SIZEWE);
    windowClass.lpszClassName = L"VCamdroidSplitter";
    RegisterClassW(&windowClass);
    return CreateWindowExW(0, windowClass.lpszClassName, L"", WS_CHILD | WS_CLIPSIBLINGS,
        0, 0, 0, 0, parent, nullptr, instance, nullptr);
}

LRESULT CALLBACK DeskHostProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_ERASEBKGND) {
        RECT client{};
        GetClientRect(window, &client);
        FillRect(reinterpret_cast<HDC>(wparam), &client, g_background);
        return 1;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

HWND CreateDeskHost(HWND parent, HINSTANCE instance) {
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = DeskHostProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"VCamdroidDeskHost";
    RegisterClassW(&windowClass);
    return CreateWindowExW(0, windowClass.lpszClassName, L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 0, 0, parent, nullptr, instance, nullptr);
}

void NotifyMoved() {
    if (g_shellController) {
        g_shellController->NotifyParentWindowPositionChanged();
    }
    if (g_deskController) {
        g_deskController->NotifyParentWindowPositionChanged();
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        const auto* created = reinterpret_cast<CREATESTRUCTW*>(lparam);
        g_preview = g_previewSurface.Create(window);
        g_splitter = CreateSplitter(window, created->hInstance);
        g_deskHost = CreateDeskHost(window, created->hInstance);
        return 0;
    }
    case kScanDoneMessage:
        ApplyScan(std::unique_ptr<ScanResult>(reinterpret_cast<ScanResult*>(lparam)));
        return 0;
    case kPollDoneMessage: {
        const std::unique_ptr<SessionPoll> poll(reinterpret_cast<SessionPoll*>(lparam));
        g_polling = false;
        ApplySession(*poll);
        return 0;
    }
    case kInstallDoneMessage:
        FinishCameraInstall(static_cast<HRESULT>(static_cast<DWORD>(wparam)));
        return 0;
    case kTipDoneMessage:
        if (!g_pendingTipId.empty() && Connected()) {
            g_tips.doneId = g_pendingTipId;
            g_tips.doneAt = GetTickCount64();
            PushState();
        }
        return 0;
    case kUpdateDoneMessage:
        FinishUpdateJob(std::unique_ptr<UpdateJob>(reinterpret_cast<UpdateJob*>(lparam)));
        return 0;
    case WM_TIMER:
        if (wparam == kStatusTimer) {
            StartPoll();
        } else if (wparam == kScanTimer) {
            StartScan();
        } else if (wparam == kDeskRetryTimer) {
            KillTimer(window, kDeskRetryTimer);
            if (Connected() && g_desk && !g_deskUrl.empty()) {
                g_desk->Navigate(DeskTarget().c_str());
            }
        }
        return 0;
    case WM_ERASEBKGND: {
        RECT client{};
        GetClientRect(window, &client);
        FillRect(reinterpret_cast<HDC>(wparam), &client,
            g_mica ? static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)) : g_background);
        return 1;
    }
    case WM_SIZE:
        if (wparam == SIZE_MINIMIZED) {
            if (g_shellController) {
                g_shellController->put_IsVisible(FALSE);
            }
            if (g_deskController) {
                g_deskController->put_IsVisible(FALSE);
            }
        } else {
            Layout();
        }
        return 0;
    case WM_MOVE:
        NotifyMoved();
        return 0;
    case WM_EXITSIZEMOVE:
        SaveWindowPlacement();
        return 0;
    case WM_SETFOCUS:
        if (g_shellController) {
            g_shellController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        return 0;
    case WM_GETMINMAXINFO: {
        auto* limits = reinterpret_cast<MINMAXINFO*>(lparam);
        limits->ptMinTrackSize.x = Scale(window, 960);
        limits->ptMinTrackSize.y = Scale(window, 640);
        return 0;
    }
    case WM_DPICHANGED: {
        const auto* suggested = reinterpret_cast<RECT*>(lparam);
        SetWindowPos(window, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        Layout();
        return 0;
    }
    case WM_DESTROY:
        SaveWindowPlacement();
        KillTimer(window, kStatusTimer);
        KillTimer(window, kScanTimer);
        KillTimer(window, kDeskRetryTimer);
        vcam::Pipeline::Shared().Stop();
        usbmux::stop();
        if (g_deskController) {
            g_deskController->Close();
        }
        if (g_shellController) {
            g_shellController->Close();
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wparam, lparam);
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int show) {
    if (commandLine != nullptr && wcsstr(commandLine, kInstallSwitch) != nullptr) {
        return static_cast<int>(vcam::InstallCameraSource());
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WSADATA data{};
    WSAStartup(MAKEWORD(2, 2), &data);

    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) {
        WSACleanup();
        return 1;
    }
    const bool ownsCom = SUCCEEDED(com);
    g_background = CreateSolidBrush(kBg);
    g_address = LoadSetting(L"panel-address.txt");
    g_obsPassword = LoadSetting(L"panel-obs-password.txt");
    g_onTop = LoadSetting(L"panel-on-top.txt") != L"0";
    LoadDeskRatio();
    g_deskLayout = LoadSetting(L"desk-layout.json");
    if (!ValidDeskLayout(g_deskLayout)) {
        g_deskLayout.clear();
    }

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"VCamdroidControls";
    RegisterClassW(&windowClass);

    g_window = CreateWindowExW(
        (g_onTop ? WS_EX_TOPMOST : 0) | WS_EX_APPWINDOW,
        windowClass.lpszClassName,
        L"VCamdroid",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        800,
        nullptr,
        nullptr,
        instance,
        nullptr
    );
    g_mica = ApplyWindowMaterial(g_window);
    if (!RestoreWindowPlacement(show)) {
        ShowWindow(g_window, show);
    }
    UpdateWindow(g_window);
    StartWebView();
    vcam::Pipeline::Shared().Prepare();
    ShowIdleStatus();
    SetTimer(g_window, kScanTimer, kScanIntervalMs, nullptr);
    StartScan();

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    vcam::Pipeline::Shared().Stop();
    usbmux::stop();
    if (g_scanThread.joinable()) {
        g_scanThread.join();
    }
    if (g_pollThread.joinable()) {
        g_pollThread.join();
    }
    if (g_installThread.joinable()) {
        g_installThread.detach();
    }
    if (g_tipThread.joinable()) {
        g_tipThread.join();
    }
    if (g_updateThread.joinable()) {
        g_updateThread.join();
    }
    if (g_background) {
        DeleteObject(g_background);
    }
    if (ownsCom) {
        CoUninitialize();
    }
    WSACleanup();
    return 0;
}
