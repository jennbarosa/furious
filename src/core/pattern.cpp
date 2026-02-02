#include "furious/core/pattern.hpp"

namespace furious {

std::optional<float> Pattern::value_at(int subdivision, PatternTargetProperty prop) const {
    for (const auto& trigger : triggers) {
        if (trigger.subdivision_index == subdivision && trigger.target == prop) {
            return trigger.value;
        }
    }
    return std::nullopt;
}

} // namespace furious
