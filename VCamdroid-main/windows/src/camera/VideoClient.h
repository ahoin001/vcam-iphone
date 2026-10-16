#pragma once

#include <string>

namespace vcam {

enum class LinkState {
    Idle,
    Connecting,
    Live,
    Interrupted
};

// Reads the phone's port 4747 video socket. While Start() is in effect it retries
// a closed socket a few times, then waits for the user to Connect again.
class VideoClient {
public:
    VideoClient();
    ~VideoClient();
    VideoClient(const VideoClient&) = delete;
    VideoClient& operator=(const VideoClient&) = delete;

    void Start(const std::string& host, int port);
    void Stop();
    LinkState State() const;

private:
    struct Worker;
    void Run(std::string host, int port);

    Worker* worker_ = nullptr;
    LinkState state_ = LinkState::Idle;
};

}  // namespace vcam
