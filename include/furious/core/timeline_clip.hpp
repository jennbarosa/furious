#pragma once

#include "furious/core/pattern.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace furious {

struct ClipEffect {
    std::string effect_id;
    std::unordered_map<std::string, std::string> parameters;
    bool enabled = true;
};

struct TimelineClip {
    std::string id;
    std::string source_id;
    size_t track_index = 0;
    double start_beat = 0.0;
    double duration_beats = 4.0;

    double source_start_seconds = 0.0;

    float position_x = 0.0f;
    float position_y = 0.0f;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    float rotation = 0.0f;

    std::vector<ClipEffect> effects;
    std::vector<ClipPatternReference> patterns;

    [[nodiscard]] double end_beat() const { return start_beat + duration_beats; }

    [[nodiscard]] bool contains_beat(double beat) const {
        return beat >= start_beat && beat < end_beat();
    }
};

} // namespace furious
