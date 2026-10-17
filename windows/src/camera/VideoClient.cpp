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

constexpr uint32_t kMaxPacket = 8u * 1024u * 1024u;
constexpr int kBackoffMs[] = {1000, 2000, 5000};
constexpr int kBackoffCount = 3;

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

void CloseLive(std::atomic<SOCKET>* live) {
    const SOCKET socket = live->exchange(INVALID_SOCKET);
    if (socket != INVALID_SOCKET) {
        shutdown(socket, SD_BOTH);
        closesocket(socket);
    }
}

}  // namespace

struct VideoClient::Worker {
    std::thread thread;
    std::atomic<bool> stop{false};
    std::atomic<SOCKET> live{INVALID_SOCKET};
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
    CloseLive(&worker->live);
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
    const auto pump = [this, worker](SOCKET socket) -> bool {
        H264Decoder decoder;
        std::vector<uint8_t> pending;
        std::vector<uint8_t> frame;
        while (!worker->stop.load()) {
            uint8_t chunk[16 * 1024];
            const int read = recv(socket, reinterpret_cast<char*>(chunk), sizeof(chunk), 0);
            if (read == 0) {
                return false;
            }
            if (read < 0) {
                const int error = WSAGetLastError();
                if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                    continue;
                }
                return false;
            }
            pending.insert(pending.end(), chunk, chunk + read);
            while (pending.size() >= 12 && !worker->stop.load()) {
                const uint32_t length = ReadBe32(pending.data() + 8);
                if (length > kMaxPacket) {
                    return false;
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
        }
        return true;
    };

    int attempt = 0;
    while (!worker->stop.load()) {
        {
            std::lock_guard<std::mutex> lock(worker->mutex);
            if (state_ != LinkState::Live) {
                state_ = LinkState::Connecting;
            }
        }
        SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket == INVALID_SOCKET) {
            break;
        }
        BOOL noDelay = TRUE;
        setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));
        DWORD timeout = 1000;
        setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        DWORD sendTimeout = 3000;
        setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendTimeout), sizeof(sendTimeout));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(port));
        worker->live.store(socket);
        if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1 ||
            connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
            CloseLive(&worker->live);
            if (worker->stop.load()) {
                return;
            }
        } else {
            const std::string request = "GET /video?1920x1080&client=panel HTTP/1.1\r\nHost: " + host + ":" +
                                        std::to_string(port) + "\r\nConnection: keep-alive\r\n\r\n";
            const bool sent = send(socket, request.data(), static_cast<int>(request.size()), 0) > 0;
            const bool finished = sent && pump(socket);
            CloseLive(&worker->live);
            if (worker->stop.load() || finished) {
                return;
            }
            attempt = 0;
        }
        if (attempt >= kBackoffCount) {
            break;
        }
        const int wait = kBackoffMs[attempt];
        ++attempt;
        for (int slept = 0; slept < wait && !worker->stop.load(); slept += 100) {
            Sleep(100);
        }
    }
    if (!worker->stop.load()) {
        std::lock_guard<std::mutex> lock(worker->mutex);
        state_ = LinkState::Interrupted;
    }
}

}  // namespace vcam
