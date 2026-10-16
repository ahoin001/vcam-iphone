#pragma once

#include "VideoClient.h"

#include <string>

namespace vcam {

class Pipeline {
public:
    static Pipeline& Shared();

    std::wstring Prepare();
    bool Available() const { return available_; }
    void Start(const std::string& host, int port);
    void Stop();
    LinkState State() const;

private:
    Pipeline() = default;

    VideoClient client_;
    std::wstring warning_;
    bool available_ = false;
    bool prepared_ = false;
};

}  // namespace vcam
