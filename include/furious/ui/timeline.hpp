#pragma once

#include "furious/core/project.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/video/source_library.hpp"
#include <string>

struct ImVec2;

namespace furious {

enum class DragMode { None, Move, TrimLeft, TrimRight };

class Timeline {
public:
    explicit Timeline(Project& project);

    void set_timeline_data(TimelineData* data) { timeline_data_ = data; }
    void set_source_library(SourceLibrary* library) { source_library_ = library; }

    [[nodiscard]] const std::string& selected_clip_id() const { return selected_clip_id_; }
    void set_selected_clip_id(const std::string& id) { selected_clip_id_ = id; }
    void clear_selection() { selected_clip_id_.clear(); }

    void render();
    void update(double delta_seconds, bool is_playing);

    void set_playhead_position(double beats);
    [[nodiscard]] double playhead_position() const;

    void set_zoom(float zoom);
    [[nodiscard]] float zoom() const;

    void set_scroll_offset(float offset);
    [[nodiscard]] float scroll_offset() const;

    void set_zoom_y(float zoom);
    [[nodiscard]] float zoom_y() const;

    void set_scroll_offset_y(float offset);
    [[nodiscard]] float scroll_offset_y() const;

    [[nodiscard]] double screen_x_to_beats(float screen_x, float canvas_x) const;

    [[nodiscard]] bool is_seeking() const { return is_seeking_; }
    [[nodiscard]] bool consume_play_toggle_request();
    [[nodiscard]] bool consume_delete_request(std::string& out_clip_id);
    [[nodiscard]] bool consume_data_modified();

    void set_follow_playhead(bool follow) { follow_playhead_ = follow; }
    [[nodiscard]] bool follow_playhead() const { return follow_playhead_; }

    void set_clip_duration_beats(double beats) { clip_duration_beats_ = beats; }
    [[nodiscard]] double clip_duration_beats() const { return clip_duration_beats_; }

    void set_fps(double fps) { fps_ = fps; }
    [[nodiscard]] double fps() const { return fps_; }

    void ensure_playhead_visible();

private:
    Project& project_;
    TimelineData* timeline_data_ = nullptr;
    SourceLibrary* source_library_ = nullptr;

    double playhead_beats_ = 0.0;
    float zoom_ = 1.0f;
    float zoom_y_ = 1.0f;
    float scroll_offset_ = 0.0f;
    float scroll_offset_y_ = 0.0f;
    float last_canvas_width_ = 800.0f;
    float last_canvas_height_ = 200.0f;
    bool is_seeking_ = false;
    bool play_toggle_requested_ = false;
    bool delete_requested_ = false;
    std::string delete_clip_id_;
    bool data_modified_ = false;
    bool follow_playhead_ = true;
    double clip_duration_beats_ = 0.0;
    double fps_ = 30.0;

    std::string selected_clip_id_;
    std::string dragging_clip_id_;
    bool dragging_clip_ = false;
    float drag_start_beat_ = 0.0f;

    DragMode drag_mode_ = DragMode::None;
    double drag_initial_start_beat_ = 0.0;
    double drag_initial_duration_ = 0.0;
    double drag_initial_source_start_ = 0.0;
    float drag_mouse_start_x_ = 0.0f;  

    static constexpr float EDGE_HIT_ZONE = 8.0f; 

    bool show_remove_track_popup_ = false;
    size_t pending_remove_track_index_ = 0;

    static constexpr float TRACK_HEIGHT = 32.0f;
    static constexpr float TRACK_SPACING = 2.0f;
    static constexpr float TRACK_HEADER_WIDTH = 80.0f;

    void render_track_headers(ImVec2 canvas_pos, float canvas_height);
    void render_tracks(ImVec2 canvas_pos, float canvas_width, float canvas_height);
    void render_clips(ImVec2 canvas_pos, float canvas_width, float canvas_height);
    void render_grid(ImVec2 canvas_pos, float canvas_width, float canvas_height);
    void render_clip_region(ImVec2 canvas_pos, float canvas_width, float canvas_height);
    void render_playhead(ImVec2 canvas_pos, float canvas_width, float canvas_height);
    void render_time_info();
    void handle_input(ImVec2 canvas_pos, float canvas_width);
    void handle_clip_interaction(ImVec2 canvas_pos, float canvas_width, float canvas_height);
};

} // namespace furious
