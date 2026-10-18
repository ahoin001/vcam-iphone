#pragma once

#include <string>
#include <vector>

namespace vcam {

struct AdviceInput {
    double fps = 0;
    int phoneDrops = 0;
    int captureDrops = 0;
    bool repeatsClimbing = false;
    int delayMs = -1;
    int cpuPercent = 0;
    bool usb = false;
    std::string thermal;
    bool portrait = false;
    bool manualExposure = false;
    double shutter = 0;
    bool torch = false;
    bool lowPower = false;
    bool charging = false;
    int battery = -1;
};

struct Tip {
    std::string id;
    std::wstring title;
    std::wstring why;
    std::wstring actionLabel;
    std::string actionPath;
};

std::vector<Tip> Advise(const AdviceInput& input);

}  // namespace vcam
