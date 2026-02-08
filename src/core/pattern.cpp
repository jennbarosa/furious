#include "furious/core/pattern.hpp"

namespace furious {

PatternPropertySettings& Pattern::settings_for(PatternTargetProperty prop) {
    switch (prop) {
        case PatternTargetProperty::PositionX: return position_x_settings;
        case PatternTargetProperty::PositionY: return position_y_settings;
        case PatternTargetProperty::ScaleX: return scale_x_settings;
        case PatternTargetProperty::ScaleY: return scale_y_settings;
        case PatternTargetProperty::Rotation: return rotation_settings;
        case PatternTargetProperty::FlipH: return flip_h_settings;
        case PatternTargetProperty::FlipV: return flip_v_settings;
    }
    return scale_x_settings;
}

const PatternPropertySettings& Pattern::settings_for(PatternTargetProperty prop) const {
    switch (prop) {
        case PatternTargetProperty::PositionX: return position_x_settings;
        case PatternTargetProperty::PositionY: return position_y_settings;
        case PatternTargetProperty::ScaleX: return scale_x_settings;
        case PatternTargetProperty::ScaleY: return scale_y_settings;
        case PatternTargetProperty::Rotation: return rotation_settings;
        case PatternTargetProperty::FlipH: return flip_h_settings;
        case PatternTargetProperty::FlipV: return flip_v_settings;
    }
    return scale_x_settings;
}

std::optional<float> Pattern::value_at(int subdivision, PatternTargetProperty prop) const {
    for (const auto& trigger : triggers) {
        if (trigger.subdivision_index == subdivision && trigger.target == prop) {
            return trigger.value;
        }
    }
    return std::nullopt;
}

} // namespace furious
