#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vcam {

// Turns Annex-B H.264 packets from the phone into tightly packed 1920x1080 NV12.
class H264Decoder {
public:
    H264Decoder();
    ~H264Decoder();
    H264Decoder(const H264Decoder&) = delete;
    H264Decoder& operator=(const H264Decoder&) = delete;

    bool Decode(uint64_t pts, const uint8_t* annexB, size_t size, std::vector<uint8_t>& nv12);

private:
    bool EnsureDecoder();
    bool Configure();
    bool ChooseOutput();
    bool Drain(std::vector<uint8_t>& nv12);

    void* decoder_ = nullptr;
    std::vector<uint8_t> sps_;
    std::vector<uint8_t> pps_;
    bool configured_ = false;
    bool inputSet_ = false;
    bool providesSamples_ = false;
    unsigned int outputBytes_ = 0;
};

}  // namespace vcam
