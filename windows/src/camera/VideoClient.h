#pragma once

#include <string>

namespace vcam {

enum class LinkState {
    Idle,
    Connecting,
    Live,
    Interrupted
};

struct LinkProgress {
    LinkState state = LinkState::Idle;
    unsigned long long bytesReceived = 0;
    unsigned long long lastByteMs = 0;
    unsigned long long lastDecodedMs = 0;
    unsigned int decodeFailures = 0;
    bool closedWithoutVideo = false;
    unsigned long long lastCaptureMs = 0;
    unsigned int decodedFrames = 0;
    unsigned int repeats = 0;
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
    LinkProgress Progress() const;

private:
    struct Worker;
    void Run(std::string host, int port);
    void NoteIfStalled(Worker* worker);

    Worker* worker_ = nullptr;
    LinkState state_ = LinkState::Idle;
};

}  // namespace vcam
