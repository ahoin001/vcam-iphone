#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <string>

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
    void Paint(HDC dc, const RECT& client);
    void ConvertNv12();
    void UpdateFont();

    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr;
    std::wstring placeholder_ = L"On the iPhone, open VCamdroid, tap OBS Studio, and leave that screen in front.";
    bool haveFrame_ = false;
    uint32_t drawnSequence_ = 0;
    void* bus_ = nullptr;
    void* nv12_ = nullptr;
    void* bgra_ = nullptr;
};

}  // namespace vcam
