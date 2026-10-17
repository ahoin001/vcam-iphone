#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace vcam {

// {C1A7E5B2-4D63-4F08-9A21-7E6B0C8D5F34}
constexpr GUID kSourceClsid = {
    0xc1a7e5b2, 0x4d63, 0x4f08, {0x9a, 0x21, 0x7e, 0x6b, 0x0c, 0x8d, 0x5f, 0x34}
};

constexpr wchar_t kSourceClsidText[] = L"{C1A7E5B2-4D63-4F08-9A21-7E6B0C8D5F34}";
constexpr wchar_t kCameraName[] = L"VCamdroid";

}  // namespace vcam
