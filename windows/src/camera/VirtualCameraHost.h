#pragma once

#include <string>

namespace vcam {

// Result of trying to register the current-user VCamdroid camera.
// listed means the camera name is in the system list.
// detail is empty when it is listed, and a sentence to show when it is not.
struct CameraSetup {
    bool windows11 = false;
    bool sourceDll = false;
    bool listed = false;
    std::wstring detail;
};

CameraSetup PrepareVirtualCamera();

}  // namespace vcam
