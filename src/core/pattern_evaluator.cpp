#include "furious/core/pattern_evaluator.hpp"
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>

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

void calculate_loop_info(
    const Pattern& pattern,
    double total_subdivisions,
    int subdivision_index,
    double subdivisions_per_beat,
    PatternEvaluationResult& result
) {
    std::vector<int> restart_subdivs;
    for (const auto& trigger : pattern.triggers) {
        const auto& settings = pattern.settings_for(trigger.target);
        if (!settings.restart_on_trigger) continue;

        bool found = false;
        for (int s : restart_subdivs) {
            if (s == trigger.subdivision_index) {
                found = true;
                break;
            }
        }
        if (!found) {
            restart_subdivs.push_back(trigger.subdivision_index);
        }
    }

    if (restart_subdivs.empty()) return;

    std::sort(restart_subdivs.begin(), restart_subdivs.end());

    int most_recent_trigger_subdiv = restart_subdivs.back();
    int next_trigger_subdiv = restart_subdivs[0];

    for (size_t i = 0; i < restart_subdivs.size(); ++i) {
        if (restart_subdivs[i] <= subdivision_index) {
            most_recent_trigger_subdiv = restart_subdivs[i];
            next_trigger_subdiv = restart_subdivs[(i + 1) % restart_subdivs.size()];
        }
    }

    int interval;
    if (next_trigger_subdiv > most_recent_trigger_subdiv) {
        interval = next_trigger_subdiv - most_recent_trigger_subdiv;
    } else {
        interval = pattern.length_subdivisions - most_recent_trigger_subdiv + next_trigger_subdiv;
    }

    double position_in_loop = total_subdivisions - most_recent_trigger_subdiv;
    while (position_in_loop < 0) position_in_loop += pattern.length_subdivisions;
    position_in_loop = std::fmod(position_in_loop, static_cast<double>(interval));

    result.use_looped_playback = true;
    result.loop_duration_beats = interval / subdivisions_per_beat;
    result.position_in_loop_beats = position_in_loop / subdivisions_per_beat;
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

        constexpr std::array<PatternTargetProperty, 7> held_properties = {
            PatternTargetProperty::PositionX,
            PatternTargetProperty::PositionY,
            PatternTargetProperty::ScaleX,
            PatternTargetProperty::ScaleY,
            PatternTargetProperty::Rotation,
            PatternTargetProperty::FlipH,
            PatternTargetProperty::FlipV
        };

        for (auto prop : held_properties) {
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

        for (const auto& trigger : pattern->triggers) {
            if (trigger.subdivision_index == subdivision_index) {
                const auto& settings = pattern->settings_for(trigger.target);
                if (settings.restart_on_trigger) {
                    result.restart_clip = true;
                    break;
                }
            }
        }

        if (!result.use_looped_playback) {
            calculate_loop_info(*pattern, total_subdivisions, subdivision_index, subdivisions_per_beat, result);
        }
    }

    return result;
}

} // namespace furious
