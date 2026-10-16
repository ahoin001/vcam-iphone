#include <winsock2.h>
#include <ws2tcpip.h>

#include "VideoClient.h"

#include "FrameBus.h"
#include "H264Decoder.h"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

namespace vcam {
namespace {

constexpr uint64_t kNoPts = ~uint64_t{0};
constexpr uint32_t kMaxPacket = 8u * 1024u * 1024u;

uint64_t ReadBe64(const uint8_t* bytes) {
    uint64_t value = 0;
    for (int index = 0; index < 8; ++index) {
        value = (value << 8) | bytes[index];
    }
    return value;
}

uint32_t ReadBe32(const uint8_t* bytes) {
    return (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) |
           (static_cast<uint32_t>(bytes[2]) << 8) | bytes[3];
}

void Publish(const std::vector<uint8_t>& frame, FrameBus* localBus, FrameBus* globalBus) {
    if (frame.size() != FrameByteCount()) {
        return;
    }
    if (globalBus != nullptr && !globalBus->IsOpen()) {
        globalBus->Create(kGlobalFrameBusName);
    }
    if (localBus != nullptr && localBus->IsOpen()) {
        localBus->Publish(frame.data(), FrameByteCount());
    }
    if (globalBus != nullptr && globalBus->IsOpen()) {
        globalBus->Publish(frame.data(), FrameByteCount());
    }
}

}  // namespace

struct VideoClient::Worker {
    std::thread thread;
    std::atomic<bool> stop{false};
    std::mutex mutex;
    FrameBus localBus;
    FrameBus globalBus;
};

VideoClient::VideoClient() {
    worker_ = new Worker();
}

VideoClient::~VideoClient() {
    Stop();
    delete static_cast<Worker*>(worker_);
    worker_ = nullptr;
}

void VideoClient::Start(const std::string& host, int port) {
    Stop();
    auto* worker = static_cast<Worker*>(worker_);
    worker->stop.store(false);
    {
        std::lock_guard<std::mutex> lock(worker->mutex);
        state_ = LinkState::Connecting;
    }
    if (!worker->localBus.IsOpen()) {
        worker->localBus.Create(kLocalFrameBusName);
    }
    worker->thread = std::thread([this, host, port]() { Run(host, port); });
}

void VideoClient::Stop() {
    auto* worker = static_cast<Worker*>(worker_);
    if (worker == nullptr) {
        return;
    }
    worker->stop.store(true);
    if (worker->thread.joinable()) {
        worker->thread.join();
    }
    std::lock_guard<std::mutex> lock(worker->mutex);
    state_ = LinkState::Idle;
}

LinkState VideoClient::State() const {
    auto* worker = static_cast<Worker*>(worker_);
    if (worker == nullptr) {
        return LinkState::Idle;
    }
    std::lock_guard<std::mutex> lock(worker->mutex);
    return state_;
}

void VideoClient::Run(std::string host, int port) {
    auto* worker = static_cast<Worker*>(worker_);
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET) {
        std::lock_guard<std::mutex> lock(worker->mutex);
        state_ = LinkState::Interrupted;
        return;
    }
    BOOL noDelay = TRUE;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));
    DWORD timeout = 1000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1 ||
        connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        closesocket(socket);
        std::lock_guard<std::mutex> lock(worker->mutex);
        state_ = LinkState::Interrupted;
        return;
    }

    const std::string request = "GET /video?1920x1080&client=panel HTTP/1.1\r\nHost: " + host + ":" +
                                std::to_string(port) + "\r\nConnection: keep-alive\r\n\r\n";
    if (send(socket, request.data(), static_cast<int>(request.size()), 0) <= 0) {
        closesocket(socket);
        std::lock_guard<std::mutex> lock(worker->mutex);
        state_ = LinkState::Interrupted;
        return;
    }

    H264Decoder decoder;
    std::vector<uint8_t> pending;
    std::vector<uint8_t> frame;
    bool interrupted = false;
    while (!worker->stop.load()) {
        uint8_t chunk[16 * 1024];
        const int read = recv(socket, reinterpret_cast<char*>(chunk), sizeof(chunk), 0);
        if (read == 0) {
            interrupted = true;
            break;
        }
        if (read < 0) {
            const int error = WSAGetLastError();
            if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                continue;
            }
            interrupted = true;
            break;
        }
        pending.insert(pending.end(), chunk, chunk + read);
        while (pending.size() >= 12 && !worker->stop.load()) {
            const uint32_t length = ReadBe32(pending.data() + 8);
            if (length > kMaxPacket) {
                pending.clear();
                interrupted = true;
                break;
            }
            if (pending.size() < 12u + length) {
                break;
            }
            const uint64_t pts = ReadBe64(pending.data());
            if (decoder.Decode(pts, pending.data() + 12, length, frame)) {
                Publish(frame, &worker->localBus, &worker->globalBus);
                std::lock_guard<std::mutex> lock(worker->mutex);
                state_ = LinkState::Live;
            }
            pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(12u + length));
        }
        if (interrupted) {
            break;
        }
    }
    closesocket(socket);
    if (interrupted && !worker->stop.load()) {
        std::lock_guard<std::mutex> lock(worker->mutex);
        state_ = LinkState::Interrupted;
    }
}

}  // namespace vcam
