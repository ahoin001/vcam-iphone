#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>

namespace vcam {

// Result of trying to list the VCamdroid camera for this user.
// listed means the camera name is in the system list.
// needsInstall means the camera component is not installed where the Windows camera
// services can load it, or the installed copy is older than the one beside VCamPanel.exe.
// detail is empty when it is listed, and a sentence to show when it is not.
struct CameraSetup {
    bool windows11 = false;
    bool sourceDll = false;
    bool installed = false;
    bool needsInstall = false;
    bool listed = false;
    std::wstring detail;
};

CameraSetup PrepareVirtualCamera();

// Copies VCamSource.dll into Program Files and registers it for the whole machine.
// Frame Server and Frame Server Monitor load the camera as system services, so they read
// HKLM and need a folder outside the user profile. Requires an elevated process.
HRESULT InstallCameraSource();

}  // namespace vcam
