#include "furious/core/timeline_data.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace furious {

TimelineData::TimelineData() {
    add_track("Track 1");
}

size_t TimelineData::add_track(std::string name) {
    Track track;
    if (name.empty()) {
        track.name = "Track " + std::to_string(tracks_.size() + 1);
    } else {
        track.name = std::move(name);
    }
    tracks_.push_back(track);
    return tracks_.size() - 1;
}

void TimelineData::remove_track(size_t index) {
    if (index >= tracks_.size()) return;

    clips_.erase(
        std::remove_if(clips_.begin(), clips_.end(),
            [index](const TimelineClip& clip) {
                return clip.track_index == index;
            }),
        clips_.end()
    );

    for (auto& clip : clips_) {
        if (clip.track_index > index) {
            clip.track_index--;
        }
    }

    tracks_.erase(tracks_.begin() + static_cast<ptrdiff_t>(index));
}

void TimelineData::add_clip(const TimelineClip& clip) {
    TimelineClip new_clip = clip;
    if (new_clip.id.empty()) {
        new_clip.id = generate_id();
    }
    clips_.push_back(new_clip);
}

void TimelineData::remove_clip(std::string_view clip_id) {
    clips_.erase(
        std::remove_if(clips_.begin(), clips_.end(),
            [clip_id](const TimelineClip& clip) {
                return clip.id == clip_id;
            }),
        clips_.end()
    );
}

void TimelineData::remove_clips_by_source(std::string_view source_id) {
    clips_.erase(
        std::remove_if(clips_.begin(), clips_.end(),
            [source_id](const TimelineClip& clip) {
                return clip.source_id == source_id;
            }),
        clips_.end()
    );
}

bool TimelineData::has_clips_using_source(std::string_view source_id) const {
    return std::any_of(clips_.begin(), clips_.end(),
        [source_id](const TimelineClip& clip) {
            return clip.source_id == source_id;
        });
}

TimelineClip* TimelineData::find_clip(std::string_view clip_id) {
    auto it = std::find_if(clips_.begin(), clips_.end(),
        [clip_id](const TimelineClip& clip) {
            return clip.id == clip_id;
        });
    return it != clips_.end() ? &(*it) : nullptr;
}

const TimelineClip* TimelineData::find_clip(std::string_view clip_id) const {
    auto it = std::find_if(clips_.begin(), clips_.end(),
        [clip_id](const TimelineClip& clip) {
            return clip.id == clip_id;
        });
    return it != clips_.end() ? &(*it) : nullptr;
}

std::vector<TimelineClip*> TimelineData::clips_at_beat(double beat) {
    std::vector<TimelineClip*> result;
    for (auto& clip : clips_) {
        if (clip.contains_beat(beat)) {
            result.push_back(&clip);
        }
    }
    std::sort(result.begin(), result.end(),
        [](const TimelineClip* a, const TimelineClip* b) {
            return a->track_index < b->track_index;
        });
    return result;
}

std::vector<TimelineClip*> TimelineData::clips_starting_between(double start_beat, double end_beat) {
    std::vector<TimelineClip*> result;
    for (auto& clip : clips_) {
        if (clip.start_beat > start_beat && clip.start_beat <= end_beat) {
            result.push_back(&clip);
        }
    }
    return result;
}

std::vector<TimelineClip*> TimelineData::clips_on_track(size_t track_index) {
    std::vector<TimelineClip*> result;
    for (auto& clip : clips_) {
        if (clip.track_index == track_index) {
            result.push_back(&clip);
        }
    }
    std::sort(result.begin(), result.end(),
        [](const TimelineClip* a, const TimelineClip* b) {
            return a->start_beat < b->start_beat;
        });
    return result;
}

std::vector<const TimelineClip*> TimelineData::clips_on_track(size_t track_index) const {
    std::vector<const TimelineClip*> result;
    for (const auto& clip : clips_) {
        if (clip.track_index == track_index) {
            result.push_back(&clip);
        }
    }
    std::sort(result.begin(), result.end(),
        [](const TimelineClip* a, const TimelineClip* b) {
            return a->start_beat < b->start_beat;
        });
    return result;
}

void TimelineData::for_each_clip(std::function<void(TimelineClip&)> fn) {
    for (auto& clip : clips_) {
        fn(clip);
    }
}

size_t TimelineData::find_available_track(double start_beat, double duration_beats) const {
    double end_beat = start_beat + duration_beats;

    for (size_t track_idx = 0; track_idx < tracks_.size(); ++track_idx) {
        bool has_overlap = false;
        for (const auto& clip : clips_) {
            if (clip.track_index != track_idx) continue;

            double clip_end = clip.start_beat + clip.duration_beats;
            if (start_beat < clip_end && end_beat > clip.start_beat) {
                has_overlap = true;
                break;
            }
        }
        if (!has_overlap) {
            return track_idx;
        }
    }

    return tracks_.size();
}

void TimelineData::set_tracks(const std::vector<Track>& tracks) {
    tracks_ = tracks;
}

void TimelineData::set_clips(const std::vector<TimelineClip>& clips) {
    clips_ = clips;
}

void TimelineData::clear() {
    clips_.clear();
    tracks_.clear();
    add_track("Track 1");
}

void TimelineData::clear_all() {
    clips_.clear();
    tracks_.clear();
}

std::string TimelineData::generate_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << dis(gen);
    ss << std::setw(8) << dis(gen);
    return ss.str();
}

} // namespace furious
