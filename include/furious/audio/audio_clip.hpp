#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace furious {

class AudioClip {
public:
    AudioClip() = default;
    ~AudioClip() = default;

    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;
    AudioClip(AudioClip&&) noexcept = default;
    AudioClip& operator=(AudioClip&&) noexcept = default;

    bool load(const std::string& filepath);
    void unload();

    [[nodiscard]] bool is_loaded() const { return !samples_.empty(); }
    [[nodiscard]] const std::string& filepath() const { return filepath_; }
    [[nodiscard]] uint32_t sample_rate() const { return sample_rate_; }
    [[nodiscard]] uint32_t channels() const { return channels_; }
    [[nodiscard]] uint64_t total_frames() const { return total_frames_; }
    [[nodiscard]] double duration_seconds() const;

    [[nodiscard]] const float* data() const { return samples_.data(); }
    [[nodiscard]] size_t sample_count() const { return samples_.size(); }

private:
    std::string filepath_;
    std::vector<float> samples_;
    uint32_t sample_rate_ = 0;
    uint32_t channels_ = 0;
    uint64_t total_frames_ = 0;
};

} // namespace furious
