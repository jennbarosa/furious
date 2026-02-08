#pragma once

#include "furious/audio/audio_buffer.hpp"
#include <expected>
#include <memory>
#include <string>

namespace furious {

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;
    AudioDecoder(AudioDecoder&&) noexcept;
    AudioDecoder& operator=(AudioDecoder&&) noexcept;

    std::expected<void, std::string> open(const std::string& filepath);
    void close();

    [[nodiscard]] bool is_open() const;
    [[nodiscard]] bool has_audio_stream() const;
    [[nodiscard]] double duration_seconds() const;

    std::expected<AudioBuffer, std::string> extract_all(uint32_t target_sample_rate = 44100,
                                                         uint32_t target_channels = 2);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
