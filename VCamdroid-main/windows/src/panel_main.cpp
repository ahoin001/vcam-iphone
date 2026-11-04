#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wrl.h>
#include <WebView2.h>

#include "mdns_browse.h"
#include "usbmux.h"
#include "camera/Pipeline.h"
#include "ui/panel_shell.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr UINT kStatusTimer = 1;
constexpr COLORREF kBg = RGB(22, 22, 24);

enum class Kind { Usb, Wifi };

struct Listed {
    Kind kind = Kind::Wifi;
    int usbId = 0;
    std::string ipv4;
    int port = 4747;
    std::wstring label;
};

HWND g_window = nullptr;
HBRUSH g_background = nullptr;
ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2> g_webview;
std::wstring g_pendingUrl;
std::wstring g_statusText = L"Plug in the iPhone or pick a Wi-Fi phone, then Connect.";
std::wstring g_addressText;
std::wstring g_obsAddress = L"127.0.0.1:4747";
std::vector<Listed> g_devices;
int g_selected = -1;
bool g_usbConnected = false;
bool g_shellReady = false;
bool g_sessionOpen = false;
std::string g_connectedHost;
int g_connectedPort = 4747;

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

std::string EscapeJson(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (unsigned char c : text) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 0x20) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    return out;
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

void PushState();

void SetStatus(const std::wstring& text) {
    g_statusText = text;
    PushState();
}

void SetObs(const std::wstring& text) {
    g_obsAddress = text;
    PushState();
}

void OpenDesk(const std::wstring& url) {
    g_pendingUrl = url;
    PushState();
}

void ShowIdleDesk() {
    g_pendingUrl.clear();
    PushState();
}

void PushState() {
    if (!g_webview || !g_shellReady) {
        return;
    }
    std::ostringstream json;
    json << "{\"devices\":[";
    for (size_t i = 0; i < g_devices.size(); ++i) {
        if (i) {
            json << ',';
        }
        json << "{\"id\":" << i
             << ",\"kind\":\"" << (g_devices[i].kind == Kind::Usb ? "usb" : "wifi")
             << "\",\"label\":\"" << EscapeJson(Narrow(g_devices[i].label)) << "\"}";
    }
    json << "],\"selected\":" << g_selected
         << ",\"address\":\"" << EscapeJson(Narrow(g_addressText)) << "\""
         << ",\"status\":\"" << EscapeJson(Narrow(g_statusText)) << "\""
         << ",\"obs\":\"" << EscapeJson(Narrow(g_obsAddress)) << "\""
         << ",\"connected\":" << (g_sessionOpen ? "true" : "false")
         << ",\"deskUrl\":\"" << EscapeJson(Narrow(g_pendingUrl)) << "\"}";
    g_webview->PostWebMessageAsJson(Wide(json.str()).c_str());
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

size_t JsonValueAt(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\":";
    auto at = json.find(needle);
    if (at == std::string::npos) {
        return std::string::npos;
    }
    at += needle.size();
    while (at < json.size() && (json[at] == ' ' || json[at] == '\t')) {
        ++at;
    }
    return at;
}

int JsonInt(const std::string& json, const char* key, int fallback) {
    const auto at = JsonValueAt(json, key);
    if (at == std::string::npos) {
        return fallback;
    }
    return atoi(json.c_str() + at);
}

std::string JsonString(const std::string& json, const char* key) {
    const auto at = JsonValueAt(json, key);
    if (at == std::string::npos || at >= json.size() || json[at] != '"') {
        return {};
    }
    const auto start = at + 1;
    const auto end = json.find('"', start);
    if (end == std::string::npos) {
        return {};
    }
    return json.substr(start, end - start);
}

bool JsonBool(const std::string& json, const char* key) {
    const auto at = JsonValueAt(json, key);
    if (at == std::string::npos) {
        return false;
    }
    return json.compare(at, 4, "true") == 0;
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
    if (!g_window || !g_controller) {
        return;
    }
    RECT client{};
    GetClientRect(g_window, &client);
    RECT bounds{0, 0, client.right, client.bottom};
    g_controller->put_Bounds(bounds);
}

void ShowWebViewError() {
    MessageBoxW(
        g_window,
        L"The WebView2 Runtime is required. Install the Microsoft Edge WebView2 Runtime, then open this window again.",
        L"VCamdroid",
        MB_OK | MB_ICONERROR
    );
}

void HandleWebMessage(const std::string& json);
void RefreshDevices();
void ConnectSelected();
void Disconnect();
void UpdateSessionStatus();

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
                                const COREWEBVIEW2_COLOR background{255, 22, 22, 24};
                                controller2->put_DefaultBackgroundColor(background);
                            }
                            EventRegistrationToken messageToken{};
                            g_webview->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR json = nullptr;
                                        if (SUCCEEDED(args->get_WebMessageAsJson(&json)) && json) {
                                            HandleWebMessage(Narrow(json));
                                            CoTaskMemFree(json);
                                        }
                                        return S_OK;
                                    }
                                ).Get(),
                                &messageToken
                            );
                            Layout();
                            const std::wstring html = Wide(PanelShellHtml());
                            g_webview->NavigateToString(html.c_str());
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

void RefreshDevices() {
    HCURSOR previous = SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    Listed previousDevice;
    const bool hadSelection = g_selected >= 0 && g_selected < static_cast<int>(g_devices.size());
    if (hadSelection) {
        previousDevice = g_devices[static_cast<size_t>(g_selected)];
    }
    g_devices.clear();
    g_selected = -1;

    const usbmux::ListResult usb = usbmux::listDevices();
    for (const usbmux::Device& device : usb.devices) {
        Listed row;
        row.kind = Kind::Usb;
        row.usbId = device.id;
        row.port = 4747;
        std::string suffix = device.serial.size() > 4 ? device.serial.substr(device.serial.size() - 4) : device.serial;
        row.label = Wide(device.name);
        if (!suffix.empty()) {
            row.label += L"  " + Wide(suffix);
        }
        g_devices.push_back(std::move(row));
    }

    const std::vector<mdns::Service> wifi = mdns::browse(1000);
    for (const mdns::Service& service : wifi) {
        Listed row;
        row.kind = Kind::Wifi;
        row.ipv4 = service.ipv4;
        row.port = service.port > 0 ? service.port : 4747;
        row.label = Wide(service.name);
        if (!service.ipv4.empty()) {
            if (!row.label.empty()) {
                row.label += L"  ";
            }
            row.label += Wide(service.ipv4);
        }
        g_devices.push_back(std::move(row));
    }

    if (hadSelection) {
        for (size_t index = 0; index < g_devices.size(); ++index) {
            const Listed& row = g_devices[index];
            const bool sameUsb = previousDevice.kind == Kind::Usb && row.kind == Kind::Usb && row.usbId == previousDevice.usbId;
            const bool sameWifi = previousDevice.kind == Kind::Wifi && row.kind == Kind::Wifi && row.ipv4 == previousDevice.ipv4;
            if (sameUsb || sameWifi) {
                g_selected = static_cast<int>(index);
                break;
            }
        }
    }
    if (g_selected < 0 && !g_devices.empty()) {
        g_selected = 0;
    }

    if (!g_sessionOpen) {
        if (!g_devices.empty()) {
            SetStatus(L"Select a phone, then Connect.");
        } else if (!usb.error.empty()) {
            SetStatus(Wide(usb.error));
        } else {
            SetStatus(L"No phone found. Plug in a data cable, tap Trust This Computer, or type the Wi-Fi IP.");
        }
    }
    SetCursor(previous);
    PushState();
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

std::wstring DeskUrl(const std::wstring& hostPort) {
    return L"http://" + hostPort + L"/panel?embed=1";
}

void ConnectSelected() {
    Listed chosen;
    bool have = g_selected >= 0 && g_selected < static_cast<int>(g_devices.size());
    if (have) {
        chosen = g_devices[static_cast<size_t>(g_selected)];
    }
    std::string typedHost;
    int typedPort = 4747;
    const bool haveTyped = ParseHostPort(g_addressText, typedHost, typedPort);

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
    g_sessionOpen = false;
    KillTimer(g_window, kStatusTimer);

    if (chosen.kind == Kind::Usb) {
        std::string error;
        const int port = (haveTyped && typedHost == "127.0.0.1") ? typedPort : 4747;
        if (!usbmux::start(port, chosen.usbId, error)) {
            SetStatus(Wide(error));
            return;
        }
        g_usbConnected = true;
        g_sessionOpen = true;
        g_connectedHost = "127.0.0.1";
        g_connectedPort = port;
        SetObs(L"127.0.0.1:" + std::to_wstring(port));
        OpenDesk(DeskUrl(L"127.0.0.1:" + std::to_wstring(port)));
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
        g_sessionOpen = true;
        g_connectedHost = chosen.ipv4;
        g_connectedPort = chosen.port;
        const std::wstring host = Wide(chosen.ipv4) + L":" + std::to_wstring(chosen.port);
        g_addressText = host;
        SaveAddress(host);
        SetObs(host);
        OpenDesk(DeskUrl(host));
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
    g_sessionOpen = false;
    g_connectedHost.clear();
    ShowIdleDesk();
    SetStatus(L"Disconnected. Connect again to use the iPhone as a camera.");
}

void SelectDevice(int index) {
    if (index < 0 || index >= static_cast<int>(g_devices.size())) {
        return;
    }
    g_selected = index;
    const Listed& chosen = g_devices[static_cast<size_t>(index)];
    if (chosen.kind == Kind::Wifi && !chosen.ipv4.empty()) {
        g_addressText = Wide(chosen.ipv4) + L":" + std::to_wstring(chosen.port);
    }
    PushState();
}

void HandleWebMessage(const std::string& json) {
    const std::string cmd = JsonString(json, "cmd");
    if (cmd == "ready") {
        g_shellReady = true;
        PushState();
        return;
    }
    if (cmd == "refresh") {
        RefreshDevices();
        return;
    }
    if (cmd == "select") {
        SelectDevice(JsonInt(json, "id", g_selected));
        return;
    }
    if (cmd == "address") {
        g_addressText = Wide(JsonString(json, "value"));
        return;
    }
    if (cmd == "connect") {
        const int id = JsonInt(json, "id", g_selected);
        if (id >= 0 && id < static_cast<int>(g_devices.size())) {
            g_selected = id;
        }
        const std::string address = JsonString(json, "value");
        const std::string typed = address.empty() ? JsonString(json, "address") : address;
        if (!typed.empty()) {
            g_addressText = Wide(typed);
        }
        ConnectSelected();
        return;
    }
    if (cmd == "disconnect") {
        Disconnect();
        return;
    }
    if (cmd == "copy") {
        CopyText(g_obsAddress);
        SetStatus(L"Copied the optional OBS address.");
    }
}

void AutoConnect() {
    for (size_t index = 0; index < g_devices.size(); ++index) {
        if (g_devices[index].kind == Kind::Usb) {
            g_selected = static_cast<int>(index);
            ConnectSelected();
            return;
        }
    }
    if (!g_addressText.empty()) {
        g_selected = -1;
        ConnectSelected();
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_TIMER:
        if (wparam == kStatusTimer) {
            UpdateSessionStatus();
        }
        return 0;
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

    WSADATA data{};
    WSAStartup(MAKEWORD(2, 2), &data);

    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) {
        WSACleanup();
        return 1;
    }
    const bool ownsCom = SUCCEEDED(com);
    g_background = CreateSolidBrush(kBg);

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
        g_addressText = saved;
    }
    ShowWindow(g_window, show);
    UpdateWindow(g_window);
    StartWebView();
    const std::wstring cameraWarning = vcam::Pipeline::Shared().Prepare();
    RefreshDevices();
    AutoConnect();
    if (!vcam::Pipeline::Shared().Available() && g_connectedHost.empty() && !cameraWarning.empty()) {
        SetStatus(cameraWarning);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    vcam::Pipeline::Shared().Stop();
    usbmux::stop();
    if (g_background) {
        DeleteObject(g_background);
    }
    if (ownsCom) {
        CoUninitialize();
    }
    WSACleanup();
    return 0;
}
