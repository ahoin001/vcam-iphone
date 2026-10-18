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

// Reads the phone's port 4747 video socket. After a picture has arrived it keeps
// retrying until Stop(). A socket that closes before any picture stops, so another
// app can take the phone.
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
