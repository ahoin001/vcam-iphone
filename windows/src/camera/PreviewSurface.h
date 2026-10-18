#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace vcam {

// Paints the latest NV12 frame already published for the VCamdroid camera.
// The companion does not decode a second time and does not ask the phone for a JPEG.
class PreviewSurface {
public:
    HWND Create(HWND parent);
    HWND Handle() const { return hwnd_; }
    void SetPlaceholder(std::wstring text);

private:
    static LRESULT CALLBACK Proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void OnCreate();
    void OnTimer();
    void Watch();
    void Paint(HDC dc, const RECT& client);
    void PaintContent(HDC dc, const RECT& client);
    bool EnsureBackBuffer(HDC target, int width, int height);
    void ReleaseBackBuffer();
    void DropStaleFrame();
    void ConvertNv12();
    void UpdateFont();

    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr;
    HDC backDc_ = nullptr;
    HBITMAP backBitmap_ = nullptr;
    HGDIOBJ backPrevious_ = nullptr;
    int backWidth_ = 0;
    int backHeight_ = 0;
    unsigned long long lastFrameMs_ = 0;
    std::wstring placeholder_ = L"On the iPhone, leave VCamdroid open on the camera screen.";
    bool haveFrame_ = false;
    uint32_t drawnSequence_ = 0;
    void* bus_ = nullptr;
    void* nv12_ = nullptr;
    void* bgra_ = nullptr;
    std::mutex busLock_;
    std::thread watch_;
    std::atomic<bool> stopWatch_{false};
    HANDLE watchStop_ = nullptr;
};

}  // namespace vcam
