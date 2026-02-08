#pragma once

#include <memory>
#include <string>
#include <cstdint>

namespace furious {

class AudioBuffer;

enum class MediaType {
    Video,
    Image
};

struct MediaSource {
    std::string id;
    std::string filepath;
    std::string name;
    MediaType type = MediaType::Video;
    double duration_seconds = 0.0;
    int width = 0;
    int height = 0;
    double fps = 30.0;

    std::shared_ptr<const AudioBuffer> audio_buffer;

    [[nodiscard]] bool has_audio() const { return audio_buffer != nullptr; }
};

} // namespace furious
