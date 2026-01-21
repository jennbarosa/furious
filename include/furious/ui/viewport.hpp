#pragma once

#include "furious/core/timeline_data.hpp"
#include "furious/video/video_engine.hpp"
#include "furious/video/source_library.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace furious {

struct ClipTransformOverride {
    std::optional<float> scale_x;
    std::optional<float> scale_y;
    std::optional<float> rotation;
    std::optional<float> position_x;
    std::optional<float> position_y;
};

class Viewport {
public:
    Viewport();

    void render();

    void set_size(float width, float height);
    [[nodiscard]] float width() const;
    [[nodiscard]] float height() const;
    [[nodiscard]] bool consume_play_toggle_request();

    void set_video_engine(VideoEngine* engine) { video_engine_ = engine; }
    void set_timeline_data(TimelineData* data) { timeline_data_ = data; }
    void set_source_library(SourceLibrary* library) { source_library_ = library; }

    void set_active_clips(const std::vector<const TimelineClip*>& clips) { active_clips_ = clips; }

    void set_selected_clip_id(const std::string& id) { selected_clip_id_ = id; }
    [[nodiscard]] const std::string& selected_clip_id() const { return selected_clip_id_; }

    void set_clip_transform_override(const std::string& clip_id, const ClipTransformOverride& override) {
        transform_overrides_[clip_id] = override;
    }
    void clear_transform_overrides() { transform_overrides_.clear(); }

private:
    float width_ = 1280.0f;
    float height_ = 720.0f;
    bool play_toggle_requested_ = false;

    VideoEngine* video_engine_ = nullptr;
    TimelineData* timeline_data_ = nullptr;
    SourceLibrary* source_library_ = nullptr;

    std::vector<const TimelineClip*> active_clips_;
    std::string selected_clip_id_;
    std::string dragging_clip_id_;
    bool dragging_ = false;

    std::unordered_map<std::string, ClipTransformOverride> transform_overrides_;
};

} // namespace furious
