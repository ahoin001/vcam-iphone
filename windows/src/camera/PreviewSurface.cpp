#include "PreviewSurface.h"

#include "FrameBus.h"

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>

namespace vcam {
namespace {

constexpr UINT kPreviewFrame = WM_APP + 1;
constexpr COLORREF kStage = RGB(12, 14, 16);
constexpr COLORREF kWaiting = RGB(154, 149, 140);
constexpr int kWaitingPoints = 13;
constexpr unsigned long long kHoldLastFrameMs = 3000;

uint8_t ClampByte(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return static_cast<uint8_t>(value);
}

void Nv12ToBgra(const uint8_t* nv12, uint8_t* bgra) {
    const int width = static_cast<int>(kFrameWidth);
    const int height = static_cast<int>(kFrameHeight);
    const uint8_t* luma = nv12;
    const uint8_t* chroma = nv12 + static_cast<size_t>(kFrameStride) * kFrameHeight;
    for (int y = 0; y < height; ++y) {
        const uint8_t* yRow = luma + static_cast<size_t>(y) * kFrameStride;
        const uint8_t* uvRow = chroma + static_cast<size_t>(y / 2) * kFrameStride;
        uint8_t* out = bgra + static_cast<size_t>(y) * static_cast<size_t>(width) * 4u;
        for (int x = 0; x < width; x += 2) {
            const int u = static_cast<int>(uvRow[x]) - 128;
            const int v = static_cast<int>(uvRow[x + 1]) - 128;
            for (int pixel = 0; pixel < 2; ++pixel) {
                const int c = static_cast<int>(yRow[x + pixel]) - 16;
                const int red = (298 * c + 459 * v + 128) >> 8;
                const int green = (298 * c - 55 * u - 136 * v + 128) >> 8;
                const int blue = (298 * c + 541 * u + 128) >> 8;
                out[0] = ClampByte(blue);
                out[1] = ClampByte(green);
                out[2] = ClampByte(red);
                out[3] = 255;
                out += 4;
            }
        }
    }
}

}  // namespace

void PreviewSurface::SetPlaceholder(std::wstring text) {
    if (text == placeholder_) {
        return;
    }
    placeholder_ = std::move(text);
    if (hwnd_ != nullptr && !haveFrame_) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

HWND PreviewSurface::Create(HWND parent) {
    WNDCLASSW windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Proc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"VCamdroidPreview";
    RegisterClassW(&windowClass);

    hwnd_ = CreateWindowExW(
        0,
        windowClass.lpszClassName,
        L"",
        WS_CHILD | WS_CLIPSIBLINGS,
        0,
        0,
        0,
        0,
        parent,
        nullptr,
        windowClass.hInstance,
        this
    );
    return hwnd_;
}

void PreviewSurface::OnCreate() {
    auto* bus = new FrameBus();
    auto* nv12 = new std::vector<uint8_t>(FrameByteCount());
    auto* bgra = new std::vector<uint8_t>(static_cast<size_t>(kFrameWidth) * kFrameHeight * 4u);
    bus_ = bus;
    nv12_ = nv12;
    bgra_ = bgra;
    UpdateFont();
    watchStop_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    watch_ = std::thread([this]() { Watch(); });
}

void PreviewSurface::Watch() {
    while (!stopWatch_.load()) {
        HANDLE frame = nullptr;
        {
            std::lock_guard<std::mutex> lock(busLock_);
            auto* bus = static_cast<FrameBus*>(bus_);
            if (bus != nullptr) {
                if (!bus->IsOpen()) {
                    const bool opened = bus->Open(kLocalFrameBusName) || bus->Open(kGlobalFrameBusName);
                    if (!opened) {
                        frame = nullptr;
                    }
                }
                frame = static_cast<HANDLE>(bus->FrameEvent());
            }
        }
        HANDLE handles[2] = {watchStop_, frame};
        const DWORD count = frame == nullptr ? 1u : 2u;
        const DWORD result = WaitForMultipleObjects(count, handles, FALSE, 200);
        if (result == WAIT_OBJECT_0 || stopWatch_.load()) {
            return;
        }
        if (hwnd_ != nullptr) {
            PostMessageW(hwnd_, kPreviewFrame, 0, 0);
        }
    }
}

void PreviewSurface::UpdateFont() {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
    LOGFONTW font = metrics.lfMessageFont;
    const UINT dpi = GetDpiForWindow(hwnd_);
    font.lfHeight = -MulDiv(kWaitingPoints, dpi == 0 ? 96 : static_cast<int>(dpi), 72);
    font.lfQuality = CLEARTYPE_QUALITY;
    if (font_ != nullptr) {
        DeleteObject(font_);
    }
    font_ = CreateFontIndirectW(&font);
}

void PreviewSurface::ConvertNv12() {
    auto* nv12 = static_cast<std::vector<uint8_t>*>(nv12_);
    auto* bgra = static_cast<std::vector<uint8_t>*>(bgra_);
    if (nv12 == nullptr || bgra == nullptr || nv12->size() != FrameByteCount()) {
        return;
    }
    Nv12ToBgra(nv12->data(), bgra->data());
}

void PreviewSurface::OnTimer() {
    std::lock_guard<std::mutex> lock(busLock_);
    auto* bus = static_cast<FrameBus*>(bus_);
    auto* nv12 = static_cast<std::vector<uint8_t>*>(nv12_);
    if (bus == nullptr || nv12 == nullptr) {
        return;
    }
    if (!bus->IsOpen() && !bus->Open(kLocalFrameBusName) && !bus->Open(kGlobalFrameBusName)) {
        DropStaleFrame();
        return;
    }
    if (!bus->Alive()) {
        DropStaleFrame();
        return;
    }
    const uint32_t sequence = bus->Sequence();
    if (sequence == 0 || sequence == drawnSequence_) {
        return;
    }
    if (!bus->CopyLatest(nv12->data(), FrameByteCount())) {
        return;
    }
    ConvertNv12();
    drawnSequence_ = sequence;
    haveFrame_ = true;
    lastFrameMs_ = GetTickCount64();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

// A short network gap should freeze the last picture, not flash the waiting text.
void PreviewSurface::DropStaleFrame() {
    if (!haveFrame_ || GetTickCount64() - lastFrameMs_ < kHoldLastFrameMs) {
        return;
    }
    haveFrame_ = false;
    drawnSequence_ = 0;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

bool PreviewSurface::EnsureBackBuffer(HDC target, int width, int height) {
    if (backDc_ != nullptr && backWidth_ == width && backHeight_ == height) {
        return true;
    }
    ReleaseBackBuffer();
    backDc_ = CreateCompatibleDC(target);
    if (backDc_ == nullptr) {
        return false;
    }
    backBitmap_ = CreateCompatibleBitmap(target, width, height);
    if (backBitmap_ == nullptr) {
        DeleteDC(backDc_);
        backDc_ = nullptr;
        return false;
    }
    backPrevious_ = SelectObject(backDc_, backBitmap_);
    backWidth_ = width;
    backHeight_ = height;
    return true;
}

void PreviewSurface::ReleaseBackBuffer() {
    if (backDc_ != nullptr) {
        if (backPrevious_ != nullptr) {
            SelectObject(backDc_, backPrevious_);
        }
        DeleteDC(backDc_);
    }
    if (backBitmap_ != nullptr) {
        DeleteObject(backBitmap_);
    }
    backDc_ = nullptr;
    backBitmap_ = nullptr;
    backPrevious_ = nullptr;
    backWidth_ = 0;
    backHeight_ = 0;
}

// The stage fill and the scaled frame land in one blit; drawing them straight to the
// window lets the compositor show the dark fill between frames.
void PreviewSurface::Paint(HDC dc, const RECT& client) {
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (width <= 0 || height <= 0) {
        return;
    }
    if (!EnsureBackBuffer(dc, width, height)) {
        PaintContent(dc, client);
        return;
    }
    const RECT local{0, 0, width, height};
    PaintContent(backDc_, local);
    BitBlt(dc, client.left, client.top, width, height, backDc_, 0, 0, SRCCOPY);
}

void PreviewSurface::PaintContent(HDC dc, const RECT& client) {
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    HBRUSH stage = CreateSolidBrush(kStage);
    FillRect(dc, &client, stage);
    DeleteObject(stage);
    if (!haveFrame_) {
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, kWaiting);
        const HGDIOBJ previous = font_ != nullptr ? SelectObject(dc, font_) : nullptr;
        const int pad = width / 10;
        RECT measure{client.left + pad, client.top, client.right - pad, client.bottom};
        DrawTextW(dc, placeholder_.c_str(), -1, &measure, DT_CENTER | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
        const int textH = measure.bottom - measure.top;
        RECT text{client.left + pad, client.top + (height - textH) / 2, client.right - pad, client.top + (height + textH) / 2};
        DrawTextW(dc, placeholder_.c_str(), -1, &text, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
        if (previous != nullptr) {
            SelectObject(dc, previous);
        }
        return;
    }
    auto* bgra = static_cast<std::vector<uint8_t>*>(bgra_);
    if (bgra == nullptr || bgra->empty()) {
        return;
    }

    const int srcW = static_cast<int>(kFrameWidth);
    const int srcH = static_cast<int>(kFrameHeight);
    const double scaleW = static_cast<double>(width) / static_cast<double>(srcW);
    const double scaleH = static_cast<double>(height) / static_cast<double>(srcH);
    const double scale = scaleW < scaleH ? scaleW : scaleH;
    const int destW = static_cast<int>(srcW * scale);
    const int destH = static_cast<int>(srcH * scale);
    const int destX = client.left + (width - destW) / 2;
    const int destY = client.top + (height - destH) / 2;

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = srcW;
    info.bmiHeader.biHeight = -srcH;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    SetStretchBltMode(dc, HALFTONE);
    SetBrushOrgEx(dc, 0, 0, nullptr);
    StretchDIBits(
        dc,
        destX,
        destY,
        destW,
        destH,
        0,
        0,
        srcW,
        srcH,
        bgra->data(),
        &info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK PreviewSurface::Proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<PreviewSurface*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* created = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<PreviewSurface*>(created->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self != nullptr) {
            self->hwnd_ = hwnd;
        }
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    switch (message) {
    case WM_CREATE:
        self->OnCreate();
        return 0;
    case WM_SHOWWINDOW:
        if (wparam) {
            self->OnTimer();
        }
        return 0;
    case kPreviewFrame:
        self->OnTimer();
        return 0;
    case WM_DPICHANGED_AFTERPARENT:
        self->UpdateFont();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd, &paint);
        RECT client{};
        GetClientRect(hwnd, &client);
        self->Paint(dc, client);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_DESTROY:
        self->stopWatch_.store(true);
        if (self->watchStop_ != nullptr) {
            SetEvent(self->watchStop_);
        }
        if (self->watch_.joinable()) {
            self->watch_.join();
        }
        if (self->watchStop_ != nullptr) {
            CloseHandle(self->watchStop_);
            self->watchStop_ = nullptr;
        }
        self->ReleaseBackBuffer();
        delete static_cast<FrameBus*>(self->bus_);
        delete static_cast<std::vector<uint8_t>*>(self->nv12_);
        delete static_cast<std::vector<uint8_t>*>(self->bgra_);
        if (self->font_ != nullptr) {
            DeleteObject(self->font_);
            self->font_ = nullptr;
        }
        self->bus_ = nullptr;
        self->nv12_ = nullptr;
        self->bgra_ = nullptr;
        self->hwnd_ = nullptr;
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
}

}  // namespace vcam
