#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace furious {

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    bool open(const std::string& filepath);
    void close();
    [[nodiscard]] bool is_open() const;

    bool seek_and_decode(double timestamp_seconds, std::vector<uint8_t>& rgba_buffer);
    bool decode_next_frame(std::vector<uint8_t>& rgba_buffer);

    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;
    [[nodiscard]] double fps() const;
    [[nodiscard]] double duration_seconds() const;
    [[nodiscard]] int64_t total_frames() const;
    [[nodiscard]] std::string decoder_type() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace furious
