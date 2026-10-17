#pragma once

#include "VideoClient.h"
#include "VirtualCameraHost.h"

#include <string>

namespace vcam {

class Pipeline {
public:
    static Pipeline& Shared();

    std::wstring Prepare();
    std::wstring Refresh();
    bool Available() const { return available_; }
    std::wstring Warning() const { return warning_; }
    CameraSetup Setup() const { return setup_; }
    void Start(const std::string& host, int port);
    void Stop();
    LinkState State() const;

private:
    Pipeline() = default;

    VideoClient client_;
    CameraSetup setup_;
    std::wstring warning_;
    bool available_ = false;
    bool prepared_ = false;
};

}  // namespace vcam
