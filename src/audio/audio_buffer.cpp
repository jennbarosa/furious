#include "furious/audio/audio_buffer.hpp"

namespace furious {

AudioBuffer::AudioBuffer(std::vector<float> samples, uint32_t sample_rate, uint32_t channels)
    : samples_(std::move(samples))
    , sample_rate_(sample_rate)
    , channels_(channels) {}

std::span<const float> AudioBuffer::samples() const {
    return samples_;
}

uint64_t AudioBuffer::frame_count() const {
    if (channels_ == 0) return 0;
    return samples_.size() / channels_;
}

double AudioBuffer::duration_seconds() const {
    if (sample_rate_ == 0) return 0.0;
    return static_cast<double>(frame_count()) / static_cast<double>(sample_rate_);
}

float AudioBuffer::sample_at(uint64_t frame, uint32_t channel) const {
    if (channel >= channels_) return 0.0f;
    size_t index = frame * channels_ + channel;
    if (index >= samples_.size()) return 0.0f;
    return samples_[index];
}

}
