#include "frame_advice.h"

namespace vcam {
namespace {

constexpr size_t kMaxTips = 3;

bool Hot(const std::string& thermal) {
    return thermal == "serious" || thermal == "critical";
}

bool Dropping(const AdviceInput& input) {
    return input.phoneDrops > 0 || input.captureDrops > 0 || input.repeatsClimbing ||
           input.delayMs > 150 || (input.fps > 0 && input.fps < 27);
}

void Add(std::vector<Tip>& tips, Tip tip) {
    if (tips.size() >= kMaxTips) {
        return;
    }
    tips.push_back(std::move(tip));
}

}  // namespace

std::vector<Tip> Advise(const AdviceInput& input) {
    std::vector<Tip> tips;
    const bool hot = Hot(input.thermal);
    const bool dropping = Dropping(input);

    if (input.portrait && (dropping || hot)) {
        Add(tips, {"portrait", L"Turn off Portrait",
                   L"Background blur runs on the phone for every frame. NVIDIA Broadcast or OBS Background Removal does it on the PC.",
                   L"Turn off Portrait", "/v1/controls?portrait=0"});
    }
    if (input.manualExposure && input.shutter > 1.0 / 30.0) {
        Add(tips, {"shutter", L"Use 1/60 shutter",
                   L"A slower shutter forces fewer than 30 frames a second.",
                   L"Use 1/60", "/v1/controls?manualexposure=1&shutter=0.016667"});
    }
    if (hot) {
        if (input.torch) {
            Add(tips, {"torch", L"Turn off torch",
                       L"The light is heating the phone and slowing the camera.",
                       L"Turn off torch", "/v1/controls?torch=0"});
        } else {
            Add(tips, {"heat", L"Cool the iPhone",
                       L"The phone is hot, so it skips frames. Take it out of the case and out of the sun.",
                       L"", ""});
        }
    }
    if (input.lowPower) {
        Add(tips, {"lowPower", L"Turn off Low Power Mode",
                   L"Low Power Mode slows the camera. Open Settings, then Battery, and turn it off.",
                   L"", ""});
    }
    if (input.battery >= 0 && input.battery < 20 && !input.charging) {
        Add(tips, {"battery", L"Plug the iPhone in",
                   L"A low battery slows the camera. Keep it charging while you record.",
                   L"", ""});
    }
    if (input.fps > 0 && input.fps < 27 && input.cpuPercent >= 85 && input.captureDrops == 0) {
        Add(tips, {"pcBusy", L"Let the GPU encode",
                   L"This PC is busy, so frames arrive late. In OBS, set the recording encoder to NVENC, AMF, or QuickSync, and close other heavy apps.",
                   L"", ""});
    }
    if (input.phoneDrops > 0 && input.cpuPercent > 0 && input.cpuPercent < 70) {
        if (input.usb) {
            Add(tips, {"usbPort", L"Use a USB port on the PC",
                       L"Frames are waiting on the phone. Plug into a port on the PC, not a hub, or try another data cable.",
                       L"", ""});
        } else {
            Add(tips, {"wifiLink", L"Use USB for recording",
                       L"Wi-Fi is falling behind. A USB cable is the steadier path for a recording.",
                       L"", ""});
        }
    }
    if (input.repeatsClimbing) {
        Add(tips, {"obsFps", L"Set OBS to 30 fps",
                   L"The recording app is asking for frames faster than they arrive. In OBS, set Video, then Common FPS Values, to 30.",
                   L"", ""});
    }
    if (tips.empty() && input.captureDrops > 0) {
        Add(tips, {"phoneApps", L"Close other apps on the phone",
                   L"The iPhone is skipping frames before they are encoded.",
                   L"", ""});
    }
    return tips;
}

}  // namespace vcam
