#pragma once

#include "furious/core/track.hpp"
#include "furious/core/timeline_clip.hpp"
#include "furious/core/media_source.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <functional>

namespace furious {

class TimelineData {
public:
    TimelineData();

    size_t add_track(std::string name = "");
    void remove_track(size_t index);
    [[nodiscard]] size_t track_count() const { return tracks_.size(); }
    [[nodiscard]] Track& track(size_t index) { return tracks_[index]; }
    [[nodiscard]] const Track& track(size_t index) const { return tracks_[index]; }

    void add_clip(const TimelineClip& clip);
    void remove_clip(std::string_view clip_id);
    void remove_clips_by_source(std::string_view source_id);
    [[nodiscard]] bool has_clips_using_source(std::string_view source_id) const;
    [[nodiscard]] TimelineClip* find_clip(std::string_view clip_id);
    [[nodiscard]] const TimelineClip* find_clip(std::string_view clip_id) const;

    [[nodiscard]] std::vector<TimelineClip*> clips_at_beat(double beat);
    [[nodiscard]] std::vector<TimelineClip*> clips_starting_between(double start_beat, double end_beat);
    [[nodiscard]] std::vector<TimelineClip*> clips_on_track(size_t track_index);
    [[nodiscard]] std::vector<const TimelineClip*> clips_on_track(size_t track_index) const;
    [[nodiscard]] size_t find_available_track(double start_beat, double duration_beats) const;

    [[nodiscard]] const std::vector<TimelineClip>& clips() const { return clips_; }
    [[nodiscard]] std::vector<TimelineClip>& clips() { return clips_; }

    void for_each_clip(std::function<void(TimelineClip&)> fn);

    void set_tracks(const std::vector<Track>& tracks);
    void set_clips(const std::vector<TimelineClip>& clips);
    [[nodiscard]] const std::vector<Track>& tracks() const { return tracks_; }

    void clear();
    void clear_all();

private:
    std::vector<Track> tracks_;
    std::vector<TimelineClip> clips_;

    static std::string generate_id();
};

} // namespace furious
