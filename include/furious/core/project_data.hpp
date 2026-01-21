#pragma once

#include "furious/core/media_source.hpp"
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
    std::string audio_filepath;
    double clip_start_seconds = 0.0;
    double clip_end_seconds = 0.0;

    // Timeline data
    std::vector<MediaSource> sources;
    std::vector<Track> tracks;
    std::vector<TimelineClip> clips;

    [[nodiscard]] bool save_to_file(const std::string& filepath) const;
    [[nodiscard]] static bool load_from_file(const std::string& filepath, ProjectData& out_data);
};

} // namespace furious
