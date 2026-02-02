#pragma once

#include "furious/core/pattern.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace furious {

class PatternLibrary {
public:
    PatternLibrary() = default;

    std::string create_pattern(const std::string& name = "New Pattern");
    void add_pattern(const Pattern& pattern);
    void remove_pattern(std::string_view pattern_id);

    [[nodiscard]] Pattern* find_pattern(std::string_view pattern_id);
    [[nodiscard]] const Pattern* find_pattern(std::string_view pattern_id) const;

    [[nodiscard]] const std::vector<Pattern>& patterns() const { return patterns_; }
    [[nodiscard]] std::vector<Pattern>& patterns() { return patterns_; }
    [[nodiscard]] size_t pattern_count() const { return patterns_.size(); }

    void clear();

    std::string duplicate_pattern(std::string_view pattern_id);

    [[nodiscard]] static std::string generate_id();

private:
    std::vector<Pattern> patterns_;
};

} // namespace furious
