#include "furious/core/pitch_library.hpp"
#include <algorithm>

namespace furious {

std::string PitchLibrary::create_track(const std::string& name) {
    PitchTrack track;
    track.id = "pitch_" + std::to_string(next_id_++);
    track.name = name.empty() ? "Pitch Track " + std::to_string(next_id_ - 1) : name;
    track.length_subdivisions = 16;
    tracks_.push_back(std::move(track));
    return tracks_.back().id;
}

std::string PitchLibrary::duplicate_track(const std::string& id) {
    const PitchTrack* source = find_track(id);
    if (!source) {
        return "";
    }
    PitchTrack copy = *source;
    copy.id = "pitch_" + std::to_string(next_id_++);
    copy.name = source->name + " (copy)";
    tracks_.push_back(std::move(copy));
    return tracks_.back().id;
}

void PitchLibrary::add_track(const PitchTrack& track) {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
        [&track](const PitchTrack& t) { return t.id == track.id; });
    if (it == tracks_.end()) {
        tracks_.push_back(track);
        if (track.id.rfind("pitch_", 0) == 0) {
            int id_num = std::stoi(track.id.substr(6));
            if (id_num >= next_id_) {
                next_id_ = id_num + 1;
            }
        }
    }
}

void PitchLibrary::remove_track(const std::string& id) {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
        [&id](const PitchTrack& t) { return t.id == id; });
    if (it != tracks_.end()) {
        tracks_.erase(it);
    }
}

PitchTrack* PitchLibrary::find_track(const std::string& id) {
    for (auto& track : tracks_) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

const PitchTrack* PitchLibrary::find_track(const std::string& id) const {
    for (const auto& track : tracks_) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

void PitchLibrary::clear() {
    tracks_.clear();
    next_id_ = 1;
}

} // namespace furious
