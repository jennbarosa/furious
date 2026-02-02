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

struct Pattern {
    std::string id;
    std::string name;
    int length_subdivisions = 16;
    std::vector<PatternTrigger> triggers;

    [[nodiscard]] std::optional<float> value_at(int subdivision, PatternTargetProperty prop) const;
};

struct ClipPatternReference {
    std::string pattern_id;
    bool enabled = true;
    int offset_subdivisions = 0;
};

} // namespace furious
