#pragma once

#include "furious/core/media_source.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace furious {

class VideoEngine {
public:
    VideoEngine();
    ~VideoEngine();

    VideoEngine(const VideoEngine&) = delete;
    VideoEngine& operator=(const VideoEngine&) = delete;

    bool initialize();
    void shutdown();

    void register_source(const MediaSource& source);
    void unregister_source(const std::string& source_id);

    void begin_frame();

    void request_frame(const std::string& clip_id, const std::string& source_id, double local_seconds);

    void request_looped_frame(const std::string& clip_id, const std::string& source_id,
                              double source_start_seconds, double loop_duration_seconds,
                              double position_in_loop);

    void prefetch_clip(const std::string& clip_id, const std::string& source_id, double start_seconds);

    void prebuild_loop_cache(const std::string& clip_id, const std::string& source_id,
                             double source_start_seconds, double loop_duration_seconds);

    [[nodiscard]] bool is_clip_cached(const std::string& clip_id) const;

    [[nodiscard]] bool is_loop_cache_complete(const std::string& clip_id) const;

    [[nodiscard]] uint32_t get_texture(const std::string& clip_id) const;
    [[nodiscard]] int get_texture_width(const std::string& source_id) const;
    [[nodiscard]] int get_texture_height(const std::string& source_id) const;

    [[nodiscard]] double get_source_duration(const std::string& source_id) const;
    [[nodiscard]] double get_source_fps(const std::string& source_id) const;

    void set_playing(bool playing);
    [[nodiscard]] bool is_playing() const { return is_playing_; }

    void update();

    [[nodiscard]] std::string get_active_decoder_info() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::atomic<bool> is_playing_{false};
};

} // namespace furious
