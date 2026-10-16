#pragma once

#include <string>

namespace vcam {

// Registers the current-user VCamdroid camera. An empty return means the camera
// is listed. A returned sentence is the status to show when this PC cannot host it.
std::wstring PrepareVirtualCamera();

}  // namespace vcam
