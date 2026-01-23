#include "furious/video/video_engine.hpp"
#include "furious/video/video_decoder.hpp"
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <cstdio>

namespace furious {

struct SourceState {
    std::unique_ptr<VideoDecoder> decoder;
    int width = 0;
    int height = 0;
    MediaType type = MediaType::Video;
};

constexpr double MAX_DECODE_RATE = 30.0;
constexpr double MIN_DECODE_INTERVAL = 1.0 / MAX_DECODE_RATE;

struct ClipState {
    std::string source_id;
    uint32_t texture_id = 0;
    int width = 0;
    int height = 0;
    std::vector<uint8_t> frame_buffer;
    double last_requested_time = -1.0;
    double last_decode_wall_time = 0.0;
    bool texture_needs_update = false;
    bool requested_this_frame = false;
    bool has_valid_frame = false;
    bool prebuilt = false;

    double loop_source_start = 0.0;
    double loop_duration = 0.0;
    double loop_frame_duration = 0.0;
    double loop_next_decode_time = 0.0;
    bool loop_cache_complete = false;
    std::vector<std::vector<uint8_t>> loop_frames;
    size_t current_loop_frame_index = 0; 
    bool use_loop_frame = false;         
};

struct VideoEngine::Impl {
    std::unordered_map<std::string, SourceState> sources;
    std::unordered_map<std::string, ClipState> clips;
    std::unordered_set<std::string> active_clip_ids;
    bool initialized = false;
};

VideoEngine::VideoEngine() : impl_(std::make_unique<Impl>()) {}

VideoEngine::~VideoEngine() {
    shutdown();
}

bool VideoEngine::initialize() {
    impl_->initialized = true;
    return true;
}

void VideoEngine::shutdown() {
    for (auto& [id, state] : impl_->clips) {
        if (state.texture_id != 0) {
            glDeleteTextures(1, &state.texture_id);
        }
    }
    impl_->clips.clear();

    for (auto& [id, state] : impl_->sources) {
        if (state.decoder) {
            state.decoder->close();
        }
    }
    impl_->sources.clear();
    impl_->initialized = false;
}

void VideoEngine::register_source(const MediaSource& source) {
    if (impl_->sources.count(source.id) > 0) {
        return;
    }

    SourceState state;
    state.type = source.type;

    if (source.type == MediaType::Video) {
        state.decoder = std::make_unique<VideoDecoder>();
        if (!state.decoder->open(source.filepath)) {
            return;
        }
        state.width = state.decoder->width();
        state.height = state.decoder->height();
        if (state.width <= 0 || state.height <= 0) {
            state.decoder->close();
            return;
        }
    } else {
        state.width = source.width > 0 ? source.width : 256;
        state.height = source.height > 0 ? source.height : 256;
    }

    impl_->sources[source.id] = std::move(state);
}

void VideoEngine::unregister_source(const std::string& source_id) {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return;

    for (auto clip_it = impl_->clips.begin(); clip_it != impl_->clips.end();) {
        if (clip_it->second.source_id == source_id) {
            if (clip_it->second.texture_id != 0) {
                glDeleteTextures(1, &clip_it->second.texture_id);
            }
            clip_it = impl_->clips.erase(clip_it);
        } else {
            ++clip_it;
        }
    }

    if (it->second.decoder) {
        it->second.decoder->close();
    }
    impl_->sources.erase(it);
}

void VideoEngine::begin_frame() {
    for (auto& [id, state] : impl_->clips) {
        state.requested_this_frame = false;
    }
    impl_->active_clip_ids.clear();
}

void VideoEngine::request_frame(const std::string& clip_id, const std::string& source_id, double local_seconds) {
    auto t_start = std::chrono::high_resolution_clock::now();

    auto source_it = impl_->sources.find(source_id);
    if (source_it == impl_->sources.end()) {
        return;
    }

    SourceState& source = source_it->second;

    if (source.type == MediaType::Image) {
        impl_->active_clip_ids.insert(clip_id);
        return;
    }

    if (source.width <= 0 || source.height <= 0) return;

    auto clip_it = impl_->clips.find(clip_id);
    if (clip_it == impl_->clips.end()) {
        auto t_tex_start = std::chrono::high_resolution_clock::now();
        ClipState clip_state;
        clip_state.source_id = source_id;
        clip_state.width = source.width;
        clip_state.height = source.height;

        glGenTextures(1, &clip_state.texture_id);
        glBindTexture(GL_TEXTURE_2D, clip_state.texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     clip_state.width, clip_state.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        impl_->clips[clip_id] = std::move(clip_state);
        clip_it = impl_->clips.find(clip_id);
        auto t_tex_end = std::chrono::high_resolution_clock::now();
        auto tex_ms = std::chrono::duration<double, std::milli>(t_tex_end - t_tex_start).count();
        if (tex_ms > 5.0) {
            std::printf("[PROFILE] request_frame texture_create: %.2fms clip=%s\n", tex_ms, clip_id.c_str());
        }
    }

    ClipState& clip = clip_it->second;
    clip.requested_this_frame = true;
    clip.use_loop_frame = false;
    impl_->active_clip_ids.insert(clip_id);

    double fps = source.decoder ? source.decoder->fps() : 30.0;
    if (fps <= 0.0) fps = 30.0;
    double frame_duration = 1.0 / fps;

    int64_t last_frame = static_cast<int64_t>(clip.last_requested_time / frame_duration);
    int64_t curr_frame = static_cast<int64_t>(local_seconds / frame_duration);

    if (last_frame == curr_frame && clip.has_valid_frame) {
        return;
    }

    clip.last_requested_time = local_seconds;

    auto t_decode_start = std::chrono::high_resolution_clock::now();
    if (source.decoder && source.decoder->seek_and_decode(local_seconds, clip.frame_buffer)) {
        auto t_decode_end = std::chrono::high_resolution_clock::now();
        auto decode_ms = std::chrono::duration<double, std::milli>(t_decode_end - t_decode_start).count();
        if (decode_ms > 10.0) {
            std::printf("[PROFILE] request_frame seek_and_decode: %.2fms clip=%s time=%.3fs\n",
                       decode_ms, clip_id.c_str(), local_seconds);
        }

        int decoder_width = source.decoder->width();
        int decoder_height = source.decoder->height();
        if (decoder_width != clip.width || decoder_height != clip.height) {
            clip.width = decoder_width;
            clip.height = decoder_height;
            if (clip.texture_id != 0) {
                glDeleteTextures(1, &clip.texture_id);
            }
            glGenTextures(1, &clip.texture_id);
            glBindTexture(GL_TEXTURE_2D, clip.texture_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                         clip.width, clip.height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        clip.texture_needs_update = true;
        clip.has_valid_frame = true;
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    if (total_ms > 16.0) {
        std::printf("[PROFILE] request_frame TOTAL: %.2fms clip=%s\n", total_ms, clip_id.c_str());
    }
}

void VideoEngine::prefetch_clip(const std::string& clip_id, const std::string& source_id, double start_seconds) {
    auto source_it = impl_->sources.find(source_id);
    if (source_it == impl_->sources.end()) return;

    SourceState& source = source_it->second;
    if (source.type == MediaType::Image) return;
    if (source.width <= 0 || source.height <= 0) return;

    auto clip_it = impl_->clips.find(clip_id);
    if (clip_it != impl_->clips.end()) return;

    ClipState clip_state;
    clip_state.source_id = source_id;
    clip_state.width = source.width;
    clip_state.height = source.height;

    glGenTextures(1, &clip_state.texture_id);
    glBindTexture(GL_TEXTURE_2D, clip_state.texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 clip_state.width, clip_state.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (source.decoder && source.decoder->seek_and_decode(start_seconds, clip_state.frame_buffer)) {
        clip_state.texture_needs_update = true;
        clip_state.has_valid_frame = true;
        clip_state.last_requested_time = start_seconds;
    }

    clip_state.prebuilt = true;
    impl_->clips[clip_id] = std::move(clip_state);
}

bool VideoEngine::is_clip_cached(const std::string& clip_id) const {
    return impl_->clips.find(clip_id) != impl_->clips.end();
}

bool VideoEngine::is_loop_cache_complete(const std::string& clip_id) const {
    auto it = impl_->clips.find(clip_id);
    if (it == impl_->clips.end()) return false;
    return it->second.loop_cache_complete;
}

void VideoEngine::prebuild_loop_cache(const std::string& clip_id, const std::string& source_id,
                                       double source_start_seconds, double loop_duration_seconds) {
    auto source_it = impl_->sources.find(source_id);
    if (source_it == impl_->sources.end()) {
        return;
    }

    SourceState& source = source_it->second;
    if (source.type == MediaType::Image) return;
    if (!source.decoder) return;
    if (source.width <= 0 || source.height <= 0) return;

    auto clip_it = impl_->clips.find(clip_id);
    if (clip_it == impl_->clips.end()) {
        ClipState clip_state;
        clip_state.source_id = source_id;
        clip_state.width = source.width;
        clip_state.height = source.height;

        glGenTextures(1, &clip_state.texture_id);
        glBindTexture(GL_TEXTURE_2D, clip_state.texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     clip_state.width, clip_state.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        impl_->clips[clip_id] = std::move(clip_state);
        clip_it = impl_->clips.find(clip_id);
    }

    ClipState& clip = clip_it->second;

    double fps = source.decoder->fps();
    if (fps <= 0.0) fps = 30.0;

    clip.loop_frames.clear();
    clip.loop_source_start = source_start_seconds;
    clip.loop_duration = loop_duration_seconds;
    clip.loop_frame_duration = 1.0 / fps;
    clip.loop_next_decode_time = source_start_seconds;
    clip.loop_cache_complete = false;

    size_t buffer_size = static_cast<size_t>(source.width) * static_cast<size_t>(source.height) * 4;
    std::vector<uint8_t> frame_buffer(buffer_size);
    double end_time = source_start_seconds + loop_duration_seconds + clip.loop_frame_duration;

    while (clip.loop_next_decode_time < end_time && clip.loop_frames.size() < 120) {
        if (source.decoder->seek_and_decode(clip.loop_next_decode_time, frame_buffer)) {
            clip.loop_frames.push_back(std::move(frame_buffer));
            frame_buffer.resize(buffer_size);
        }
        clip.loop_next_decode_time += clip.loop_frame_duration;
    }

    clip.loop_cache_complete = true;

    int decoder_width = source.decoder->width();
    int decoder_height = source.decoder->height();
    if (decoder_width != clip.width || decoder_height != clip.height) {
        clip.width = decoder_width;
        clip.height = decoder_height;
        if (clip.texture_id != 0) {
            glDeleteTextures(1, &clip.texture_id);
        }
        glGenTextures(1, &clip.texture_id);
        glBindTexture(GL_TEXTURE_2D, clip.texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     clip.width, clip.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (!clip.loop_frames.empty()) {
        clip.current_loop_frame_index = 0;
        clip.use_loop_frame = true;
        clip.has_valid_frame = true;
        clip.texture_needs_update = true;
    }

    clip.prebuilt = true;
}

void VideoEngine::request_looped_frame(const std::string& clip_id, const std::string& source_id,
                                        double source_start_seconds, double loop_duration_seconds,
                                        double position_in_loop) {
    auto t_start = std::chrono::high_resolution_clock::now();

    auto source_it = impl_->sources.find(source_id);
    if (source_it == impl_->sources.end()) return;

    SourceState& source = source_it->second;

    if (source.type == MediaType::Image) {
        impl_->active_clip_ids.insert(clip_id);
        return;
    }

    if (!source.decoder) return;

    if (source.width <= 0 || source.height <= 0) return;

    auto clip_it = impl_->clips.find(clip_id);
    if (clip_it == impl_->clips.end()) {
        ClipState clip_state;
        clip_state.source_id = source_id;
        clip_state.width = source.width;
        clip_state.height = source.height;

        glGenTextures(1, &clip_state.texture_id);
        glBindTexture(GL_TEXTURE_2D, clip_state.texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     clip_state.width, clip_state.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        impl_->clips[clip_id] = std::move(clip_state);
        clip_it = impl_->clips.find(clip_id);
    }

    ClipState& clip = clip_it->second;
    clip.requested_this_frame = true;
    impl_->active_clip_ids.insert(clip_id);

    bool params_changed = (clip.loop_source_start != source_start_seconds) ||
                          (clip.loop_duration != loop_duration_seconds);

    if (is_interactive_ && params_changed) {
        return;
    }

    if (params_changed) {
        double fps = source.decoder->fps();
        if (fps <= 0.0) fps = 30.0;

        clip.loop_frames.clear();
        clip.loop_source_start = source_start_seconds;
        clip.loop_duration = loop_duration_seconds;
        clip.loop_frame_duration = 1.0 / fps;
        clip.loop_next_decode_time = source_start_seconds;
        clip.loop_cache_complete = false;
    }

    if (!clip.loop_cache_complete && clip.loop_frames.size() < 120) {
        auto t_cache_start = std::chrono::high_resolution_clock::now();
        constexpr size_t MAX_FRAMES_PER_CALL = 5;

        size_t buffer_size = static_cast<size_t>(source.width) * static_cast<size_t>(source.height) * 4;
        std::vector<uint8_t> frame_buffer(buffer_size);
        double end_time = clip.loop_source_start + clip.loop_duration + clip.loop_frame_duration;
        size_t frames_decoded = 0;

        while (clip.loop_next_decode_time < end_time && frames_decoded < MAX_FRAMES_PER_CALL) {
            if (source.decoder->seek_and_decode(clip.loop_next_decode_time, frame_buffer)) {
                clip.loop_frames.push_back(std::move(frame_buffer));
                frame_buffer.resize(buffer_size);
            }
            clip.loop_next_decode_time += clip.loop_frame_duration;
            ++frames_decoded;

            if (clip.loop_frames.size() >= 120) break;
        }
        auto t_cache_end = std::chrono::high_resolution_clock::now();
        auto cache_ms = std::chrono::duration<double, std::milli>(t_cache_end - t_cache_start).count();
        if (cache_ms > 10.0) {
            std::printf("[PROFILE] request_looped_frame cache_decode: %.2fms frames=%zu clip=%s\n",
                       cache_ms, frames_decoded, clip_id.c_str());
        }

        if (clip.loop_next_decode_time >= end_time || clip.loop_frames.size() >= 120) {
            clip.loop_cache_complete = true;
        }

        int decoder_width = source.decoder->width();
        int decoder_height = source.decoder->height();
        if (decoder_width != clip.width || decoder_height != clip.height) {
            clip.width = decoder_width;
            clip.height = decoder_height;
            if (clip.texture_id != 0) {
                glDeleteTextures(1, &clip.texture_id);
            }
            glGenTextures(1, &clip.texture_id);
            glBindTexture(GL_TEXTURE_2D, clip.texture_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                         clip.width, clip.height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    if (!clip.loop_frames.empty() && clip.loop_frame_duration > 0.0) {
        size_t index = static_cast<size_t>(position_in_loop / clip.loop_frame_duration);
        if (index >= clip.loop_frames.size()) index = clip.loop_frames.size() - 1;
        clip.current_loop_frame_index = index;
        clip.use_loop_frame = true;
        clip.texture_needs_update = true;
        clip.has_valid_frame = true;
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    if (total_ms > 16.0) {
        std::printf("[PROFILE] request_looped_frame TOTAL: %.2fms clip=%s\n", total_ms, clip_id.c_str());
    }
}

void VideoEngine::update() {
    for (auto& [id, clip] : impl_->clips) {
        if (clip.texture_needs_update) {
            const uint8_t* frame_data = nullptr;
            size_t frame_size = 0;

            if (clip.use_loop_frame && clip.current_loop_frame_index < clip.loop_frames.size()) {
                const auto& cached_frame = clip.loop_frames[clip.current_loop_frame_index];
                frame_data = cached_frame.data();
                frame_size = cached_frame.size();
            } else if (!clip.frame_buffer.empty()) {
                frame_data = clip.frame_buffer.data();
                frame_size = clip.frame_buffer.size();
            }

            size_t expected_size = static_cast<size_t>(clip.width * clip.height * 4);
            if (frame_data == nullptr || frame_size != expected_size) {
                clip.texture_needs_update = false;
                continue;
            }

            glBindTexture(GL_TEXTURE_2D, clip.texture_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            clip.width, clip.height,
                            GL_RGBA, GL_UNSIGNED_BYTE,
                            frame_data);
            glBindTexture(GL_TEXTURE_2D, 0);
            clip.texture_needs_update = false;
        }
    }

    for (auto it = impl_->clips.begin(); it != impl_->clips.end();) {
        bool is_active = impl_->active_clip_ids.find(it->first) != impl_->active_clip_ids.end();
        if (!is_active && !it->second.prebuilt) {
            if (it->second.texture_id != 0) {
                glDeleteTextures(1, &it->second.texture_id);
            }
            it = impl_->clips.erase(it);
        } else {
            ++it;
        }
    }
}

uint32_t VideoEngine::get_texture(const std::string& clip_id) const {
    auto it = impl_->clips.find(clip_id);
    if (it == impl_->clips.end()) return 0;
    if (!it->second.has_valid_frame) return 0;
    return it->second.texture_id;
}

int VideoEngine::get_texture_width(const std::string& source_id) const {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return 0;
    return it->second.width;
}

int VideoEngine::get_texture_height(const std::string& source_id) const {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return 0;
    return it->second.height;
}

double VideoEngine::get_source_duration(const std::string& source_id) const {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return 0.0;
    if (!it->second.decoder) return 0.0;
    return it->second.decoder->duration_seconds();
}

double VideoEngine::get_source_fps(const std::string& source_id) const {
    auto it = impl_->sources.find(source_id);
    if (it == impl_->sources.end()) return 0.0;
    if (!it->second.decoder) return 0.0;
    return it->second.decoder->fps();
}

void VideoEngine::set_playing(bool playing) {
    is_playing_ = playing;
}

void VideoEngine::set_interactive_mode(bool interactive) {
    is_interactive_ = interactive;
}

std::string VideoEngine::get_active_decoder_info() const {
    for (const auto& [id, state] : impl_->sources) {
        if (state.decoder && state.decoder->is_open()) {
            return state.decoder->decoder_type();
        }
    }
    return "None";
}

} // namespace furious
