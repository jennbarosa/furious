#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace furious {

class AudioBuffer {
public:
    AudioBuffer() = default;
    AudioBuffer(std::vector<float> samples, uint32_t sample_rate, uint32_t channels);

    [[nodiscard]] std::span<const float> samples() const;
    [[nodiscard]] uint32_t sample_rate() const { return sample_rate_; }
    [[nodiscard]] uint32_t channels() const { return channels_; }
    [[nodiscard]] uint64_t frame_count() const;
    [[nodiscard]] double duration_seconds() const;
    [[nodiscard]] bool empty() const { return samples_.empty(); }

    [[nodiscard]] float sample_at(uint64_t frame, uint32_t channel) const;

private:
    std::vector<float> samples_;
    uint32_t sample_rate_ = 44100;
    uint32_t channels_ = 2;
};

}
