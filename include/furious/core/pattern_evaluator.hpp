#pragma once

#include "furious/core/pattern.hpp"
#include "furious/core/pattern_library.hpp"
#include "furious/core/timeline_clip.hpp"
#include <optional>

namespace furious {

struct PatternEvaluationResult {
    std::optional<float> position_x;
    std::optional<float> position_y;
    std::optional<float> scale_x;
    std::optional<float> scale_y;
    std::optional<float> rotation;
    std::optional<bool> flip_h;
    std::optional<bool> flip_v;
    bool restart_clip = false;

    bool use_looped_playback = false;
    double loop_duration_beats = 0.0;
    double position_in_loop_beats = 0.0;
};

class PatternEvaluator {
public:
    PatternEvaluator() = default;

    void set_pattern_library(PatternLibrary* library) { library_ = library; }

    [[nodiscard]] PatternEvaluationResult evaluate(
        const TimelineClip& clip,
        double clip_local_beats
    ) const;

private:
    PatternLibrary* library_ = nullptr;
};

} // namespace furious
