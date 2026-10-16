#include "Pipeline.h"

#include "VirtualCameraHost.h"

namespace vcam {

Pipeline& Pipeline::Shared() {
    static Pipeline pipeline;
    return pipeline;
}

std::wstring Pipeline::Prepare() {
    if (prepared_) {
        return warning_;
    }
    prepared_ = true;
    warning_ = PrepareVirtualCamera();
    available_ = warning_.empty();
    return warning_;
}

void Pipeline::Start(const std::string& host, int port) {
    if (host.empty() || port <= 0) {
        return;
    }
    client_.Start(host, port);
}

void Pipeline::Stop() {
    client_.Stop();
}

LinkState Pipeline::State() const {
    return client_.State();
}

}  // namespace vcam
