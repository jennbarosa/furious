#pragma once

#include "furious/core/media_source.hpp"
#include "furious/core/pattern.hpp"
#include "furious/core/tempo.hpp"
#include "furious/core/timeline_clip.hpp"
#include "furious/core/track.hpp"
#include <string>
#include <vector>

namespace furious {

struct ProjectData {
    std::string name = "Untitled Project";
    double bpm = 120.0;
    NoteSubdivision grid_subdivision = NoteSubdivision::Quarter;
    double fps = 30.0;
    bool metronome_enabled = false;
    bool follow_playhead = true;
    bool loop_enabled = false;
    double playhead_beat = 0.0;
    float timeline_zoom = 1.0f;
    float timeline_zoom_y = 1.0f;
    float timeline_scroll = 0.0f;
    float timeline_scroll_y = 0.0f;
    int window_width = 1280;
    int window_height = 720;
    std::string imgui_layout;
    std::string audio_filepath;
    double clip_start_seconds = 0.0;
    double clip_end_seconds = 0.0;

    std::vector<MediaSource> sources;
    std::vector<Track> tracks;
    std::vector<TimelineClip> clips;
    std::vector<Pattern> patterns;

    [[nodiscard]] bool save_to_file(const std::string& filepath) const;
    [[nodiscard]] static bool load_from_file(const std::string& filepath, ProjectData& out_data);
};

} // namespace furious
