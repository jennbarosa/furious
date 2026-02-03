#include "furious/core/pattern_evaluator.hpp"
#include <cmath>
#include <array>

namespace furious {

namespace {

const PatternTrigger* find_active_trigger(
    const Pattern& pattern,
    int current_subdivision,
    PatternTargetProperty target
) {
    const PatternTrigger* best = nullptr;
    int best_distance = pattern.length_subdivisions + 1;

    for (const auto& trigger : pattern.triggers) {
        if (trigger.target != target) continue;

        int distance;
        if (trigger.subdivision_index <= current_subdivision) {
            distance = current_subdivision - trigger.subdivision_index;
        } else {
            distance = current_subdivision + (pattern.length_subdivisions - trigger.subdivision_index);
        }

        if (distance < best_distance) {
            best_distance = distance;
            best = &trigger;
        }
    }

    return best;
}

}

PatternEvaluationResult PatternEvaluator::evaluate(
    const TimelineClip& clip,
    double clip_local_beats
) const {
    PatternEvaluationResult result;

    if (!library_ || clip.patterns.empty()) {
        return result;
    }

    for (const auto& ref : clip.patterns) {
        if (!ref.enabled) continue;

        const Pattern* pattern = library_->find_pattern(ref.pattern_id);
        if (!pattern || pattern->triggers.empty()) continue;

        double subdivisions_per_beat = 4.0;
        double total_subdivisions = clip_local_beats * subdivisions_per_beat;
        total_subdivisions += ref.offset_subdivisions;

        int subdivision_index = static_cast<int>(std::fmod(total_subdivisions, pattern->length_subdivisions));
        if (subdivision_index < 0) {
            subdivision_index += pattern->length_subdivisions;
        }

        constexpr std::array<PatternTargetProperty, 7> all_properties = {
            PatternTargetProperty::PositionX,
            PatternTargetProperty::PositionY,
            PatternTargetProperty::ScaleX,
            PatternTargetProperty::ScaleY,
            PatternTargetProperty::Rotation,
            PatternTargetProperty::FlipH,
            PatternTargetProperty::FlipV
        };

        for (auto prop : all_properties) {
            const PatternTrigger* active = find_active_trigger(*pattern, subdivision_index, prop);
            if (!active) continue;

            switch (prop) {
                case PatternTargetProperty::PositionX:
                    result.position_x = active->value;
                    break;
                case PatternTargetProperty::PositionY:
                    result.position_y = active->value;
                    break;
                case PatternTargetProperty::ScaleX:
                    result.scale_x = active->value;
                    break;
                case PatternTargetProperty::ScaleY:
                    result.scale_y = active->value;
                    break;
                case PatternTargetProperty::Rotation:
                    result.rotation = active->value;
                    break;
                case PatternTargetProperty::FlipH:
                    result.flip_h = active->value != 0.0f;
                    break;
                case PatternTargetProperty::FlipV:
                    result.flip_v = active->value != 0.0f;
                    break;
            }
        }
    }

    return result;
}

} // namespace furious
