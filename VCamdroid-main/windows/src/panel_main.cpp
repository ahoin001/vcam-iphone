#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <wrl.h>
#include <WebView2.h>

#include "mdns_browse.h"
#include "usbmux.h"
#include "camera/Pipeline.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr int kTitleId = 1000;
constexpr int kListId = 1001;
constexpr int kRefreshId = 1002;
constexpr int kAddressId = 1003;
constexpr int kConnectId = 1004;
constexpr int kDisconnectId = 1005;
constexpr int kCopyId = 1006;
constexpr int kStatusId = 1007;
constexpr int kObsId = 1008;
constexpr int kDevicesLabelId = 1009;
constexpr int kLinkLabelId = 1010;
constexpr UINT kStatusTimer = 1;

constexpr COLORREF kBg = RGB(18, 20, 22);
constexpr COLORREF kCard = RGB(28, 31, 34);
constexpr COLORREF kText = RGB(242, 239, 232);
constexpr COLORREF kMuted = RGB(154, 149, 140);

enum class Kind { Usb, Wifi };

struct Listed {
    Kind kind = Kind::Wifi;
    int usbId = 0;
    std::string ipv4;
    int port = 4747;
    std::wstring label;
};

HWND g_window = nullptr;
HWND g_title = nullptr;
HWND g_devicesLabel = nullptr;
HWND g_list = nullptr;
HWND g_refresh = nullptr;
HWND g_linkLabel = nullptr;
HWND g_address = nullptr;
HWND g_connect = nullptr;
HWND g_disconnect = nullptr;
HWND g_copy = nullptr;
HWND g_status = nullptr;
HWND g_obs = nullptr;
HBRUSH g_background = nullptr;
HBRUSH g_card = nullptr;
ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2> g_webview;
std::wstring g_pendingUrl;
std::vector<Listed> g_devices;
bool g_usbConnected = false;
std::string g_connectedHost = "127.0.0.1";
int g_connectedPort = 4747;
std::wstring g_obsAddress = L"127.0.0.1:4747";
HFONT g_titleFont = nullptr;
HFONT g_uiFont = nullptr;
HFONT g_smallFont = nullptr;

int Scale(HWND window, int pixels) {
    const UINT dpi = GetDpiForWindow(window);
    return MulDiv(pixels, dpi == 0 ? 96 : static_cast<int>(dpi), 96);
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

std::wstring AddressPath() {
    const std::wstring root = EnvFolder(L"APPDATA");
    if (root.empty()) {
        return L"";
    }
    const std::wstring folder = root + L"\\VCamdroid";
    CreateDirectoryW(folder.c_str(), nullptr);
    return folder + L"\\panel-address.txt";
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

std::wstring LoadAddress() {
    const std::wstring path = AddressPath();
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

void SaveAddress(const std::wstring& text) {
    const std::wstring path = AddressPath();
    if (path.empty()) {
        return;
    }
    const std::string encoded = Narrow(text);
    std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
    output.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
}

void SetStatus(const std::wstring& text) {
    if (g_status) {
        SetWindowTextW(g_status, text.c_str());
    }
}

void SetObs(const std::wstring& text) {
    g_obsAddress = text;
    if (g_obs) {
        SetWindowTextW(g_obs, (L"VCamdroid    optional OBS  " + text).c_str());
    }
}

std::wstring IdleDeskHtml() {
    return LR"HTML(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  :root { color-scheme: dark; }
  html, body { height: 100%; margin: 0; }
  body {
    display: grid; place-items: center;
    background:
      radial-gradient(120% 80% at 50% -10%, rgba(217,160,74,0.08), transparent 55%),
      linear-gradient(180deg, #171a1d, #121416);
    color: #f2efe8;
    font: 14px/1.5 "Segoe UI", system-ui, sans-serif;
  }
  .shell {
    width: min(920px, 92vw);
    display: grid;
    grid-template-columns: minmax(0, 1.3fr) minmax(260px, 340px);
    gap: 28px;
    align-items: center;
    padding: 28px;
  }
  .stage {
    min-height: 220px;
    border-radius: 16px;
    border: 1px solid #2a3036;
    background:
      linear-gradient(180deg, #14181c, #0c0e10);
    display: grid;
    place-items: center;
    color: #9a958c;
    font-size: 13px;
  }
  .copy h1 { margin: 0 0 10px; font-size: 28px; font-weight: 600; }
  .copy p { margin: 0; color: #9a958c; font-size: 14px; }
  .steps { margin: 18px 0 0; padding: 0; list-style: none; }
  .steps li {
    display: flex; gap: 10px; align-items: flex-start;
    padding: 8px 0; border-top: 1px solid #24292e;
    color: #f2efe8; font-size: 13px;
  }
  .steps li:first-child { border-top: 0; }
  .n {
    flex: 0 0 22px; height: 22px; border-radius: 999px;
    display: grid; place-items: center;
    background: rgba(217,160,74,0.18); color: #d9a04a;
    font-size: 11px; font-weight: 700;
  }
  @media (max-width: 720px) {
    .shell { grid-template-columns: 1fr; padding: 20px 16px; }
    .stage { min-height: 140px; }
  }
</style></head><body>
  <div class="shell">
    <div class="stage">Preview opens here after Connect</div>
    <div class="copy">
      <h1>Connect a phone</h1>
      <p>This window receives the iPhone and publishes the VCamdroid camera. Shot controls sit beside the preview.</p>
      <ol class="steps">
        <li><span class="n">1</span><span>Click Refresh to find USB or Wi-Fi phones.</span></li>
        <li><span class="n">2</span><span>Select a phone, or type the Wi-Fi IP.</span></li>
        <li><span class="n">3</span><span>Click Connect. The picture opens in this window.</span></li>
      </ol>
    </div>
  </div>
</body></html>)HTML";
}

void ShowIdleDesk() {
    g_pendingUrl.clear();
    if (g_title) {
        SetWindowTextW(g_title, L"VCamdroid");
    }
    if (g_webview) {
        g_webview->NavigateToString(IdleDeskHtml().c_str());
    }
}

void Navigate(const std::wstring& url) {
    g_pendingUrl = url;
    if (g_webview) {
        g_webview->Navigate(url.c_str());
    }
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

int JsonInt(const std::string& json, const char* key, int fallback) {
    const std::string needle = std::string("\"") + key + "\":";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return fallback;
    }
    return atoi(json.c_str() + at + needle.size());
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
    if (connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
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

void Layout() {
    if (!g_window) {
        return;
    }
    RECT client{};
    GetClientRect(g_window, &client);
    const int margin = Scale(g_window, 12);
    const int gap = Scale(g_window, 8);
    const int titleH = Scale(g_window, 22);
    const int bar = Scale(g_window, 56);
    const int row = Scale(g_window, 30);
    const int label = Scale(g_window, 16);
    const int copyW = Scale(g_window, 72);
    const int refreshW = Scale(g_window, 80);
    const int connectW = Scale(g_window, 88);
    const int disconnectW = Scale(g_window, 100);
    const int listW = Scale(g_window, 260);
    const int width = client.right - client.left;
    const int inner = width - margin * 2;
    int y = margin;
    const int titleW = Scale(g_window, 200);
    MoveWindow(g_title, margin, y, titleW, titleH, TRUE);
    MoveWindow(g_status, margin + titleW + gap, y, inner - titleW - copyW - gap * 2, titleH, TRUE);
    MoveWindow(g_copy, width - margin - copyW, y, copyW, titleH, TRUE);
    y += titleH + gap;
    MoveWindow(g_devicesLabel, 0, 0, 0, 0, TRUE);
    MoveWindow(g_linkLabel, 0, 0, 0, 0, TRUE);
    MoveWindow(g_list, margin, y, listW, bar, TRUE);
    int x = margin + listW + gap;
    MoveWindow(g_refresh, x, y, refreshW, bar, TRUE);
    x += refreshW + gap;
    const int actions = connectW + disconnectW + gap;
    const int addressW = (width - margin) - x - actions - gap;
    MoveWindow(g_address, x, y + (bar - row) / 2, (std::max)(Scale(g_window, 140), addressW), row, TRUE);
    x += (std::max)(Scale(g_window, 140), addressW) + gap;
    MoveWindow(g_connect, x, y + (bar - row) / 2, connectW, row, TRUE);
    x += connectW + gap;
    MoveWindow(g_disconnect, x, y + (bar - row) / 2, disconnectW, row, TRUE);
    y += bar + Scale(g_window, 4);
    MoveWindow(g_obs, margin, y, inner, label, TRUE);
    y += label + gap;
    if (g_controller) {
        RECT bounds{0, y, client.right, client.bottom};
        g_controller->put_Bounds(bounds);
    }
}

void ShowWebViewError() {
    MessageBoxW(
        g_window,
        L"The WebView2 Runtime is required. Install the Microsoft Edge WebView2 Runtime, then open this window again.",
        L"VCamdroid",
        MB_OK | MB_ICONERROR
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
                environment->CreateCoreWebView2Controller(
                    g_window,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT created, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(created) || controller == nullptr) {
                                ShowWebViewError();
                                return S_OK;
                            }
                            g_controller = controller;
                            if (FAILED(g_controller->get_CoreWebView2(&g_webview)) || !g_webview) {
                                ShowWebViewError();
                                return S_OK;
                            }
                            ComPtr<ICoreWebView2Controller2> controller2;
                            if (SUCCEEDED(g_controller.As(&controller2)) && controller2) {
                                const COREWEBVIEW2_COLOR background{255, 18, 20, 22};
                                controller2->put_DefaultBackgroundColor(background);
                            }
                            Layout();
                            if (!g_pendingUrl.empty()) {
                                g_webview->Navigate(g_pendingUrl.c_str());
                            } else {
                                ShowIdleDesk();
                            }
                            return S_OK;
                        }
                    ).Get()
                );
                return S_OK;
            }
        ).Get()
    );
    if (FAILED(requested)) {
        ShowWebViewError();
    }
}

void AddDevice(Listed device) {
    const int index = static_cast<int>(g_devices.size());
    g_devices.push_back(std::move(device));
    SendMessageW(g_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(g_devices.back().label.c_str()));
    SendMessageW(g_list, LB_SETITEMDATA, static_cast<WPARAM>(index), static_cast<LPARAM>(index));
}

void RefreshDevices() {
    HCURSOR previous = SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    g_devices.clear();

    const usbmux::ListResult usb = usbmux::listDevices();
    for (const usbmux::Device& device : usb.devices) {
        Listed row;
        row.kind = Kind::Usb;
        row.usbId = device.id;
        row.port = 4747;
        std::string suffix = device.serial.size() > 4 ? device.serial.substr(device.serial.size() - 4) : device.serial;
        row.label = L"USB  " + Wide(device.name);
        if (!suffix.empty()) {
            row.label += L"  " + Wide(suffix);
        }
        AddDevice(std::move(row));
    }

    const std::vector<mdns::Service> wifi = mdns::browse(1000);
    for (const mdns::Service& service : wifi) {
        Listed row;
        row.kind = Kind::Wifi;
        row.ipv4 = service.ipv4;
        row.port = service.port > 0 ? service.port : 4747;
        row.label = L"Wi-Fi  " + Wide(service.name) + L"  " + Wide(service.ipv4);
        AddDevice(std::move(row));
    }

    if (!g_devices.empty()) {
        SendMessageW(g_list, LB_SETCURSEL, 0, 0);
        SetStatus(L"Select a phone, then Connect.");
    } else if (!usb.error.empty()) {
        SetStatus(Wide(usb.error));
    } else {
        SetStatus(L"No phone found. Plug in a data cable, tap Trust This Computer, or type the Wi-Fi IP.");
    }
    SetCursor(previous);
}

void UpdateSessionStatus() {
    if (g_connectedHost.empty()) {
        return;
    }
    const vcam::LinkState link = vcam::Pipeline::Shared().State();
    if (link == vcam::LinkState::Interrupted) {
        SetStatus(L"The video connection closed. Click Connect to try again.");
        return;
    }
    std::string body;
    if (!HttpGet(g_connectedHost, g_connectedPort, "/v1/session", body)) {
        SetStatus(g_usbConnected
            ? L"USB is on. Unlock the iPhone and keep VCamdroid in front."
            : L"Waiting for the phone. Keep VCamdroid in front.");
        return;
    }
    const std::string status = JsonString(body, "status");
    const int battery = JsonInt(body, "battery", -1);
    const int width = JsonInt(body, "width", 0);
    const int height = JsonInt(body, "height", 0);
    const int fps = JsonInt(body, "fps", 0);
    const bool streaming = JsonBool(body, "streaming") || status == "streaming";
    std::wstring line = g_usbConnected ? L"USB" : L"Wi-Fi";
    if (battery >= 0) {
        line += L"  " + std::to_wstring(battery) + L"%";
    }
    line += (streaming || link == vcam::LinkState::Live) ? L"  Streaming" : L"  Waiting";
    if (width > 0 && height > 0) {
        line += L"  " + std::to_wstring(width) + L"x" + std::to_wstring(height);
        if (fps > 0) {
            line += L" @" + std::to_wstring(fps);
        }
    }
    if (link == vcam::LinkState::Live) {
        line += L"  VCamdroid";
    } else if (link == vcam::LinkState::Connecting) {
        line += L"  Opening picture";
    }
    if (!vcam::Pipeline::Shared().Available() && !vcam::Pipeline::Shared().Warning().empty()) {
        line += L"  ";
        line += vcam::Pipeline::Shared().Warning();
    }
    SetStatus(line);
}

void NoteConnected(const std::wstring& address) {
    vcam::Pipeline::Shared().Start(g_connectedHost, g_connectedPort);
    if (vcam::Pipeline::Shared().Available()) {
        SetStatus(L"Connected. Choose VCamdroid as the camera.");
        return;
    }
    const std::wstring warning = vcam::Pipeline::Shared().Warning();
    if (!warning.empty()) {
        SetStatus(L"Connected. Opening the picture. " + warning);
        return;
    }
    SetStatus(L"Connected. Opening the picture. Optional OBS address " + address + L".");
}

void ConnectSelected() {
    const int selection = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    Listed chosen;
    bool have = false;
    if (selection >= 0 && selection < static_cast<int>(g_devices.size())) {
        chosen = g_devices[static_cast<size_t>(selection)];
        have = true;
    }
    wchar_t typed[256] = {};
    GetWindowTextW(g_address, typed, 256);
    std::string typedHost;
    int typedPort = 4747;
    const bool haveTyped = ParseHostPort(typed, typedHost, typedPort);

    if (!have) {
        if (!haveTyped) {
            SetStatus(L"Select a phone in the list, or type a Wi-Fi IP.");
            return;
        }
        chosen.kind = Kind::Wifi;
        chosen.ipv4 = typedHost;
        chosen.port = typedPort;
    }

    vcam::Pipeline::Shared().Stop();
    usbmux::stop();
    g_usbConnected = false;
    KillTimer(g_window, kStatusTimer);

    if (chosen.kind == Kind::Usb) {
        std::string error;
        const int port = haveTyped ? typedPort : 4747;
        if (!usbmux::start(port, chosen.usbId, error)) {
            SetStatus(Wide(error));
            return;
        }
        g_usbConnected = true;
        g_connectedHost = "127.0.0.1";
        g_connectedPort = port;
        SetObs(L"127.0.0.1:" + std::to_wstring(port));
        Navigate(L"http://127.0.0.1:" + std::to_wstring(port) + L"/panel");
        if (g_title) {
            SetWindowTextW(g_title, L"VCamdroid  ·  USB");
        }
        NoteConnected(L"127.0.0.1:" + std::to_wstring(port));
    } else {
        if (chosen.ipv4.empty() && haveTyped) {
            chosen.ipv4 = typedHost;
            chosen.port = typedPort;
        }
        if (chosen.ipv4.empty()) {
            SetStatus(L"Type the Wi-Fi IP from the phone.");
            return;
        }
        g_connectedHost = chosen.ipv4;
        g_connectedPort = chosen.port;
        const std::wstring host = Wide(chosen.ipv4) + L":" + std::to_wstring(chosen.port);
        SetWindowTextW(g_address, host.c_str());
        SaveAddress(host);
        SetObs(host);
        Navigate(L"http://" + host + L"/panel");
        if (g_title) {
            SetWindowTextW(g_title, L"VCamdroid  ·  Wi-Fi");
        }
        NoteConnected(host);
    }
    SetTimer(g_window, kStatusTimer, 1000, nullptr);
    UpdateSessionStatus();
}

void Disconnect() {
    KillTimer(g_window, kStatusTimer);
    vcam::Pipeline::Shared().Stop();
    usbmux::stop();
    g_usbConnected = false;
    g_connectedHost.clear();
    ShowIdleDesk();
    SetStatus(L"Disconnected. Connect again to use the iPhone as a camera.");
}

void LayoutFont(HWND window, HFONT font) {
    SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void AutoConnect() {
    for (size_t index = 0; index < g_devices.size(); ++index) {
        if (g_devices[index].kind == Kind::Usb) {
            SendMessageW(g_list, LB_SETCURSEL, static_cast<WPARAM>(index), 0);
            ConnectSelected();
            return;
        }
    }
    wchar_t typed[256] = {};
    GetWindowTextW(g_address, typed, 256);
    if (typed[0] != L'\0') {
        SendMessageW(g_list, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
        ConnectSelected();
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        const DWORD child = WS_CHILD | WS_VISIBLE;
        g_title = CreateWindowExW(0, L"STATIC", L"VCamdroid",
            child | SS_LEFT, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTitleId)), nullptr, nullptr);
        g_devicesLabel = CreateWindowExW(0, L"STATIC", L"Phones",
            child | SS_LEFT, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDevicesLabelId)), nullptr, nullptr);
        g_list = CreateWindowExW(0, L"LISTBOX", L"", child | WS_VSCROLL | WS_BORDER | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)), nullptr, nullptr);
        g_refresh = CreateWindowExW(0, L"BUTTON", L"Refresh", child | BS_PUSHBUTTON,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRefreshId)), nullptr, nullptr);
        g_linkLabel = CreateWindowExW(0, L"STATIC", L"Address",
            child | SS_LEFT, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLinkLabelId)), nullptr, nullptr);
        g_address = CreateWindowExW(0, L"EDIT", L"", child | WS_BORDER | ES_AUTOHSCROLL,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAddressId)), nullptr, nullptr);
        g_connect = CreateWindowExW(0, L"BUTTON", L"Connect", child | BS_DEFPUSHBUTTON,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kConnectId)), nullptr, nullptr);
        g_disconnect = CreateWindowExW(0, L"BUTTON", L"Disconnect", child | BS_PUSHBUTTON,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDisconnectId)), nullptr, nullptr);
        g_status = CreateWindowExW(0, L"STATIC", L"Plug in the iPhone or pick a Wi-Fi phone, then Connect.",
            child | SS_LEFT | SS_WORDELLIPSIS, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusId)), nullptr, nullptr);
        g_copy = CreateWindowExW(0, L"BUTTON", L"Copy", child | BS_PUSHBUTTON,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCopyId)), nullptr, nullptr);
        g_obs = CreateWindowExW(0, L"STATIC", L"VCamdroid    optional OBS  127.0.0.1:4747",
            child | SS_LEFT, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kObsId)), nullptr, nullptr);
        SendMessageW(g_address, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Wi-Fi IP, if the list is empty"));
        NONCLIENTMETRICSW metrics{};
        metrics.cbSize = sizeof(metrics);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
        g_uiFont = CreateFontIndirectW(&metrics.lfMessageFont);
        const HDC screen = GetDC(window);
        const int dpiY = GetDeviceCaps(screen, LOGPIXELSY);
        ReleaseDC(window, screen);
        LOGFONTW titleFont = metrics.lfMessageFont;
        titleFont.lfWeight = FW_SEMIBOLD;
        titleFont.lfHeight = -MulDiv(16, dpiY, 72);
        g_titleFont = CreateFontIndirectW(&titleFont);
        LOGFONTW smallFont = metrics.lfMessageFont;
        smallFont.lfHeight = -MulDiv(11, dpiY, 72);
        g_smallFont = CreateFontIndirectW(&smallFont);
        LayoutFont(g_title, g_titleFont);
        LayoutFont(g_devicesLabel, g_smallFont);
        LayoutFont(g_linkLabel, g_smallFont);
        LayoutFont(g_list, g_uiFont);
        LayoutFont(g_refresh, g_uiFont);
        LayoutFont(g_address, g_uiFont);
        LayoutFont(g_connect, g_uiFont);
        LayoutFont(g_disconnect, g_uiFont);
        LayoutFont(g_status, g_uiFont);
        LayoutFont(g_copy, g_uiFont);
        LayoutFont(g_obs, g_smallFont);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case kRefreshId:
            RefreshDevices();
            return 0;
        case kConnectId:
            ConnectSelected();
            return 0;
        case kDisconnectId:
            Disconnect();
            return 0;
        case kCopyId:
            CopyText(g_obsAddress);
            SetStatus(L"Copied the optional OBS address.");
            return 0;
        case kListId:
            if (HIWORD(wparam) == LBN_DBLCLK) {
                ConnectSelected();
            }
            return 0;
        default:
            return 0;
        }
    case WM_TIMER:
        if (wparam == kStatusTimer) {
            UpdateSessionStatus();
        }
        return 0;
    case WM_CTLCOLORSTATIC: {
        const HDC dc = reinterpret_cast<HDC>(wparam);
        const HWND child = reinterpret_cast<HWND>(lparam);
        if (child == g_devicesLabel || child == g_linkLabel || child == g_obs) {
            SetTextColor(dc, kMuted);
        } else if (child == g_title) {
            SetTextColor(dc, kText);
        } else {
            SetTextColor(dc, kText);
        }
        SetBkColor(dc, kBg);
        return reinterpret_cast<LRESULT>(g_background);
    }
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT: {
        const HDC dc = reinterpret_cast<HDC>(wparam);
        SetTextColor(dc, kText);
        SetBkColor(dc, kCard);
        return reinterpret_cast<LRESULT>(g_card);
    }
    case WM_ERASEBKGND: {
        RECT client{};
        GetClientRect(window, &client);
        FillRect(reinterpret_cast<HDC>(wparam), &client, g_background);
        return 1;
    }
    case WM_SIZE:
        Layout();
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
    case WM_DESTROY: {
        KillTimer(window, kStatusTimer);
        vcam::Pipeline::Shared().Stop();
        usbmux::stop();
        if (g_titleFont) {
            DeleteObject(g_titleFont);
            g_titleFont = nullptr;
        }
        if (g_uiFont) {
            DeleteObject(g_uiFont);
            g_uiFont = nullptr;
        }
        if (g_smallFont) {
            DeleteObject(g_smallFont);
            g_smallFont = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProcW(window, message, wparam, lparam);
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    WSADATA data{};
    WSAStartup(MAKEWORD(2, 2), &data);

    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) {
        WSACleanup();
        return 1;
    }
    const bool ownsCom = SUCCEEDED(com);
    g_background = CreateSolidBrush(kBg);
    g_card = CreateSolidBrush(kCard);

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = g_background;
    windowClass.lpszClassName = L"VCamdroidControls";
    RegisterClassW(&windowClass);

    g_window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
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
    const std::wstring saved = LoadAddress();
    if (!saved.empty()) {
        SetWindowTextW(g_address, saved.c_str());
    }
    ShowWindow(g_window, show);
    UpdateWindow(g_window);
    Layout();
    StartWebView();
    const std::wstring cameraWarning = vcam::Pipeline::Shared().Prepare();
    RefreshDevices();
    AutoConnect();
    if (!vcam::Pipeline::Shared().Available() && g_connectedHost.empty() && !cameraWarning.empty()) {
        SetStatus(cameraWarning);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN) {
            const HWND focus = GetFocus();
            if (focus == g_address || focus == g_list) {
                ConnectSelected();
                continue;
            }
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    vcam::Pipeline::Shared().Stop();
    usbmux::stop();
    if (g_background) {
        DeleteObject(g_background);
    }
    if (g_card) {
        DeleteObject(g_card);
    }
    if (ownsCom) {
        CoUninitialize();
    }
    WSACleanup();
    return 0;
}
