#include "obs_tally.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace {
std::thread g_thread;
std::atomic<bool> g_stop{false};
std::atomic<SOCKET> g_socket{INVALID_SOCKET};
std::string g_sent;

void CloseSocket() {
    const SOCKET socket = g_socket.exchange(INVALID_SOCKET);
    if (socket != INVALID_SOCKET) {
        closesocket(socket);
    }
}

std::string Base64(const uint8_t* data, size_t size) {
    static constexpr char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((size + 2) / 3 * 4);
    for (size_t index = 0; index < size; index += 3) {
        const uint32_t a = data[index];
        const uint32_t b = index + 1 < size ? data[index + 1] : 0;
        const uint32_t c = index + 2 < size ? data[index + 2] : 0;
        const uint32_t triple = (a << 16) | (b << 8) | c;
        out.push_back(kTable[(triple >> 18) & 63]);
        out.push_back(kTable[(triple >> 12) & 63]);
        out.push_back(index + 1 < size ? kTable[(triple >> 6) & 63] : '=');
        out.push_back(index + 2 < size ? kTable[triple & 63] : '=');
    }
    return out;
}

bool Sha256(const std::string& text, uint8_t out[32]) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        return false;
    }
    DWORD bytes = 0;
    DWORD objectSize = 0;
    BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &bytes, 0);
    std::vector<uint8_t> object(objectSize);
    const bool ok = BCryptCreateHash(algorithm, &hash, object.data(), objectSize, nullptr, 0, 0) == 0 &&
                    BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(text.data())), static_cast<ULONG>(text.size()), 0) == 0 &&
                    BCryptFinishHash(hash, out, 32, 0) == 0;
    if (hash != nullptr) {
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return ok;
}

std::string ObsAuthentication(const std::string& password, const std::string& salt, const std::string& challenge) {
    uint8_t first[32] = {};
    if (!Sha256(password + salt, first)) {
        return {};
    }
    const std::string secret = Base64(first, sizeof(first));
    uint8_t second[32] = {};
    if (!Sha256(secret + challenge, second)) {
        return {};
    }
    return Base64(second, sizeof(second));
}

std::string JsonString(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\":\"";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return {};
    }
    const auto start = at + needle.size();
    const auto end = json.find('"', start);
    if (end == std::string::npos) {
        return {};
    }
    return json.substr(start, end - start);
}

bool JsonBool(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\":";
    const auto at = json.find(needle);
    if (at == std::string::npos) {
        return false;
    }
    return json.compare(at + needle.size(), 4, "true") == 0;
}

bool SendAll(SOCKET socket, const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        const int wrote = send(socket, data + sent, size - sent, 0);
        if (wrote <= 0) {
            return false;
        }
        sent += wrote;
    }
    return true;
}

bool HttpPut(const std::string& host, int port, const std::string& path) {
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET) {
        return false;
    }
    DWORD timeout = 800;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1 ||
        connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        closesocket(socket);
        return false;
    }
    const std::string request = "PUT " + path + " HTTP/1.1\r\nHost: " + host + ":" + std::to_string(port) +
                                "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    const bool sent = SendAll(socket, request.data(), static_cast<int>(request.size()));
    closesocket(socket);
    return sent;
}

void PublishTally(const std::string& host, int port, const char* tally) {
    if (g_sent == tally) {
        return;
    }
    if (HttpPut(host, port, std::string("/v1/tally/") + tally + "/")) {
        g_sent = tally;
    }
}

bool SendFrame(SOCKET socket, const std::string& text) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81);
    const size_t size = text.size();
    if (size < 126) {
        frame.push_back(static_cast<uint8_t>(0x80 | size));
    } else {
        frame.push_back(0x80 | 126);
        frame.push_back(static_cast<uint8_t>((size >> 8) & 0xff));
        frame.push_back(static_cast<uint8_t>(size & 0xff));
    }
    uint8_t mask[4] = {};
    BCryptGenRandom(nullptr, mask, sizeof(mask), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    frame.insert(frame.end(), mask, mask + 4);
    for (size_t index = 0; index < size; ++index) {
        frame.push_back(static_cast<uint8_t>(text[index]) ^ mask[index % 4]);
    }
    return SendAll(socket, reinterpret_cast<const char*>(frame.data()), static_cast<int>(frame.size()));
}

int RecvWait(SOCKET socket, char* data, int size) {
    int got = 0;
    while (got < size) {
        if (g_stop.load()) {
            return -1;
        }
        const int read = recv(socket, data + got, size - got, 0);
        if (read == 0) {
            return -1;
        }
        if (read < 0) {
            const int error = WSAGetLastError();
            if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) {
                continue;
            }
            return -1;
        }
        got += read;
    }
    return got;
}

bool ReadFrame(SOCKET socket, std::string& text) {
    uint8_t header[2] = {};
    if (RecvWait(socket, reinterpret_cast<char*>(header), 2) != 2) {
        return false;
    }
    const int opcode = header[0] & 0x0f;
    uint64_t length = header[1] & 0x7f;
    if (length == 126) {
        uint8_t extended[2] = {};
        if (RecvWait(socket, reinterpret_cast<char*>(extended), 2) != 2) {
            return false;
        }
        length = (static_cast<uint64_t>(extended[0]) << 8) | extended[1];
    } else if (length == 127) {
        return false;
    }
    if (length > 1024 * 1024) {
        return false;
    }
    std::string payload(static_cast<size_t>(length), '\0');
    if (!payload.empty() && RecvWait(socket, payload.data(), static_cast<int>(payload.size())) != static_cast<int>(payload.size())) {
        return false;
    }
    if (opcode == 0x8) {
        return false;
    }
    if (opcode == 0x9) {
        return ReadFrame(socket, text);
    }
    if (opcode == 0x1 || opcode == 0x0) {
        text += payload;
        if ((header[0] & 0x80) == 0) {
            return ReadFrame(socket, text);
        }
        return true;
    }
    text.clear();
    return true;
}

std::string TallyFor(bool recording, bool streaming, bool studio) {
    if (recording || streaming) {
        return "program";
    }
    if (studio) {
        return "preview";
    }
    return "idle";
}

void NoteMessage(const std::string& message, bool* recording, bool* streaming, bool* studio) {
    const std::string type = JsonString(message, "eventType");
    const std::string request = JsonString(message, "requestType");
    if (type == "RecordStateChanged" || request == "GetRecordStatus") {
        *recording = JsonBool(message, "outputActive");
    } else if (type == "StreamStateChanged" || request == "GetStreamStatus") {
        *streaming = JsonBool(message, "outputActive");
    } else if (type == "StudioModeStateChanged" || request == "GetStudioModeEnabled") {
        *studio = JsonBool(message, "studioModeEnabled");
    }
}

void Listen(std::string host, int port, std::string password) {
    bool recording = false;
    bool streaming = false;
    bool studio = false;
    while (!g_stop.load()) {
        SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket == INVALID_SOCKET) {
            return;
        }
        DWORD timeout = 1000;
        setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        g_socket.store(socket);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(4455);
        inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
        if (connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
            CloseSocket();
            for (int slept = 0; slept < 2000 && !g_stop.load(); slept += 100) {
                Sleep(100);
            }
            continue;
        }
        uint8_t keyBytes[16] = {};
        BCryptGenRandom(nullptr, keyBytes, sizeof(keyBytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        const std::string key = Base64(keyBytes, sizeof(keyBytes));
        const std::string upgrade = "GET / HTTP/1.1\r\nHost: 127.0.0.1:4455\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                                    "Sec-WebSocket-Key: " +
                                    key + "\r\nSec-WebSocket-Version: 13\r\n\r\n";
        if (!SendAll(socket, upgrade.data(), static_cast<int>(upgrade.size()))) {
            CloseSocket();
            continue;
        }
        std::string handshake;
        char chunk[512];
        while (handshake.find("\r\n\r\n") == std::string::npos) {
            const int read = recv(socket, chunk, sizeof(chunk), 0);
            if (read <= 0) {
                break;
            }
            handshake.append(chunk, static_cast<size_t>(read));
        }
        std::string hello;
        if (handshake.find("101") == std::string::npos || !ReadFrame(socket, hello)) {
            CloseSocket();
            continue;
        }
        std::string identify = "{\"op\":1,\"d\":{\"rpcVersion\":1,\"eventSubscriptions\":1092";
        const std::string salt = JsonString(hello, "salt");
        const std::string challenge = JsonString(hello, "challenge");
        if (!salt.empty()) {
            const std::string authentication = ObsAuthentication(password, salt, challenge);
            identify += ",\"authentication\":\"" + authentication + "\"";
        }
        identify += "}}";
        if (!SendFrame(socket, identify)) {
            CloseSocket();
            continue;
        }
        SendFrame(socket, "{\"op\":6,\"d\":{\"requestType\":\"GetRecordStatus\",\"requestId\":\"rec\"}}");
        SendFrame(socket, "{\"op\":6,\"d\":{\"requestType\":\"GetStreamStatus\",\"requestId\":\"stream\"}}");
        SendFrame(socket, "{\"op\":6,\"d\":{\"requestType\":\"GetStudioModeEnabled\",\"requestId\":\"studio\"}}");
        while (!g_stop.load()) {
            std::string message;
            if (!ReadFrame(socket, message)) {
                break;
            }
            if (message.empty()) {
                continue;
            }
            NoteMessage(message, &recording, &streaming, &studio);
            PublishTally(host, port, TallyFor(recording, streaming, studio).c_str());
        }
        CloseSocket();
        recording = false;
        streaming = false;
        studio = false;
        PublishTally(host, port, "idle");
    }
}

}  // namespace

void StartObsTally(const std::string& host, int port, const std::wstring& password) {
    StopObsTally();
    g_stop.store(false);
    g_sent.clear();
    std::string utf8;
    utf8.reserve(password.size());
    for (wchar_t character : password) {
        if (character >= 32 && character < 127) {
            utf8.push_back(static_cast<char>(character));
        }
    }
    g_thread = std::thread(Listen, host, port, std::move(utf8));
}

void StopObsTally() {
    g_stop.store(true);
    CloseSocket();
    if (g_thread.joinable()) {
        g_thread.join();
    }
    g_stop.store(false);
    g_sent.clear();
}
