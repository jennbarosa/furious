#pragma once

#include <string>
#include <cstdint>

namespace furious {

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
};

} // namespace furious
