#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <wrl.h>
#include <WebView2.h>

#include <fstream>
#include <iterator>
#include <string>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr int kAddressId = 1001;
constexpr int kOpenId = 1002;

HWND g_window = nullptr;
HWND g_address = nullptr;
HWND g_open = nullptr;
ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2> g_webview;
std::wstring g_pendingUrl;

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

std::wstring Normalize(std::wstring text) {
    text = Trim(std::move(text));
    if (text.empty()) {
        return L"http://127.0.0.1:4747/panel";
    }
    if (text.find(L"://") == std::wstring::npos) {
        if (text.find(L':') == std::wstring::npos) {
            text += L":4747";
        }
        text = L"http://" + text;
    }
    while (!text.empty() && text.back() == L'/') {
        text.pop_back();
    }
    const std::wstring suffix = L"/panel";
    if (text.size() < suffix.size() || text.compare(text.size() - suffix.size(), suffix.size(), suffix) != 0) {
        text += suffix;
    }
    return text;
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
    if (bytes.empty()) {
        return L"";
    }
    const int wide = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    if (wide <= 0) {
        return L"";
    }
    std::wstring text(static_cast<size_t>(wide), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), text.data(), wide);
    return Trim(text);
}

void SaveAddress(const std::wstring& url) {
    const std::wstring path = AddressPath();
    if (path.empty()) {
        return;
    }
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, url.c_str(), static_cast<int>(url.size()), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) {
        return;
    }
    std::string encoded(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, url.c_str(), static_cast<int>(url.size()), encoded.data(), bytes, nullptr, nullptr);
    std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
    output.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
}

void Navigate(const std::wstring& url) {
    g_pendingUrl = url;
    if (g_webview) {
        g_webview->Navigate(url.c_str());
    }
}

void NavigateFromEdit() {
    wchar_t buffer[1024] = {};
    GetWindowTextW(g_address, buffer, 1024);
    const std::wstring url = Normalize(buffer);
    SetWindowTextW(g_address, url.c_str());
    SaveAddress(url);
    Navigate(url);
}

void Layout() {
    if (!g_window) {
        return;
    }
    RECT client{};
    GetClientRect(g_window, &client);
    const int margin = Scale(g_window, 8);
    const int bar = Scale(g_window, 28);
    const int button = Scale(g_window, 72);
    const int width = client.right - client.left;
    MoveWindow(g_address, margin, margin, width - button - margin * 3, bar, TRUE);
    MoveWindow(g_open, width - button - margin, margin, button, bar, TRUE);
    if (g_controller) {
        RECT bounds{0, bar + margin * 2, client.right, client.bottom};
        g_controller->put_Bounds(bounds);
    }
}

void ShowWebViewError() {
    MessageBoxW(
        g_window,
        L"The WebView2 Runtime is required. Install the Microsoft Edge WebView2 Runtime, then open this window again.",
        L"VCamdroid Controls",
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
                                const COREWEBVIEW2_COLOR background{255, 20, 22, 24};
                                controller2->put_DefaultBackgroundColor(background);
                            }
                            Layout();
                            if (!g_pendingUrl.empty()) {
                                g_webview->Navigate(g_pendingUrl.c_str());
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

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE:
        g_address = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAddressId)), nullptr, nullptr);
        g_open = CreateWindowExW(0, L"BUTTON", L"Open", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kOpenId)), nullptr, nullptr);
        SendMessageW(g_address, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"192.168.1.20:4747"));
        return 0;
    case WM_COMMAND:
        if (LOWORD(wparam) == kOpenId) {
            NavigateFromEdit();
            return 0;
        }
        return 0;
    case WM_SIZE:
        Layout();
        return 0;
    case WM_DPICHANGED: {
        const auto* suggested = reinterpret_cast<RECT*>(lparam);
        SetWindowPos(window, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        Layout();
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wparam, lparam);
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) {
        return 1;
    }
    const bool ownsCom = SUCCEEDED(com);

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = L"VCamdroidControls";
    RegisterClassW(&windowClass);

    g_window = CreateWindowExW(
        0,
        windowClass.lpszClassName,
        L"VCamdroid Controls",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        440,
        760,
        nullptr,
        nullptr,
        instance,
        nullptr
    );
    const std::wstring saved = LoadAddress();
    if (!saved.empty()) {
        g_pendingUrl = Normalize(saved);
        SetWindowTextW(g_address, g_pendingUrl.c_str());
    }
    ShowWindow(g_window, show);
    UpdateWindow(g_window);
    Layout();
    StartWebView();

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN && GetFocus() == g_address) {
            NavigateFromEdit();
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (ownsCom) {
        CoUninitialize();
    }
    return 0;
}
