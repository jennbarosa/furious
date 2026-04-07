#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace furious {

enum class YUVFormat { NV12, YUV420P, Unknown };

struct DecodedFrame {
    std::vector<uint8_t> y_plane;
    std::vector<uint8_t> uv_plane;  // NV12: interleaved UV. YUV420P: U plane only.
    std::vector<uint8_t> v_plane;   // YUV420P only.
    int y_stride = 0;
    int uv_stride = 0;
    int v_stride = 0;
    int width = 0;
    int height = 0;
    YUVFormat format = YUVFormat::Unknown;
};

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
    bool seek_and_decode_yuv(double timestamp_seconds, DecodedFrame& frame);
    bool decode_next_frame(std::vector<uint8_t>& rgba_buffer);

    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;
    [[nodiscard]] double fps() const;
    [[nodiscard]] double duration_seconds() const;
    [[nodiscard]] int64_t total_frames() const;
    [[nodiscard]] std::string decoder_type() const;
    [[nodiscard]] int pixel_format() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace furious
