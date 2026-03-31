#pragma once

#include "furious/core/pitch_track.hpp"
#include <vector>
#include <string>

namespace furious {

class PitchLibrary {
public:
    std::string create_track(const std::string& name);
    std::string duplicate_track(const std::string& id);
    void add_track(const PitchTrack& track);
    void remove_track(const std::string& id);
    PitchTrack* find_track(const std::string& id);
    const PitchTrack* find_track(const std::string& id) const;

    [[nodiscard]] const std::vector<PitchTrack>& tracks() const { return tracks_; }
    [[nodiscard]] std::vector<PitchTrack>& tracks() { return tracks_; }

    void clear();

private:
    std::vector<PitchTrack> tracks_;
    int next_id_ = 1;
};

} // namespace furious
