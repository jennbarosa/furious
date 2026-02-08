#pragma once

#include <optional>
#include <string>
#include <vector>

namespace furious {

enum class PatternTargetProperty {
    PositionX,
    PositionY,
    ScaleX,
    ScaleY,
    Rotation,
    FlipH,
    FlipV
};

struct PatternTrigger {
    int subdivision_index = 0;
    PatternTargetProperty target = PatternTargetProperty::ScaleX;
    float value = 1.0f;
};

struct PatternPropertySettings {
    bool restart_on_trigger = false;
};

struct Pattern {
    std::string id;
    std::string name;
    int length_subdivisions = 16;
    std::vector<PatternTrigger> triggers;

    PatternPropertySettings position_x_settings;
    PatternPropertySettings position_y_settings;
    PatternPropertySettings scale_x_settings;
    PatternPropertySettings scale_y_settings;
    PatternPropertySettings rotation_settings;
    PatternPropertySettings flip_h_settings;
    PatternPropertySettings flip_v_settings;

    [[nodiscard]] PatternPropertySettings& settings_for(PatternTargetProperty prop);
    [[nodiscard]] const PatternPropertySettings& settings_for(PatternTargetProperty prop) const;
    [[nodiscard]] std::optional<float> value_at(int subdivision, PatternTargetProperty prop) const;
};

struct ClipPatternReference {
    std::string pattern_id;
    bool enabled = true;
    int offset_subdivisions = 0;
};

} // namespace furious
