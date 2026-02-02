#include "furious/core/pattern_library.hpp"
#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>

namespace furious {

std::string PatternLibrary::create_pattern(const std::string& name) {
    Pattern pattern;
    pattern.id = generate_id();
    pattern.name = name;
    pattern.length_subdivisions = 16;
    patterns_.push_back(pattern);
    return pattern.id;
}

void PatternLibrary::add_pattern(const Pattern& pattern) {
    patterns_.push_back(pattern);
}

void PatternLibrary::remove_pattern(std::string_view pattern_id) {
    std::erase_if(patterns_, [pattern_id](const Pattern& p) {
        return p.id == pattern_id;
    });
}

Pattern* PatternLibrary::find_pattern(std::string_view pattern_id) {
    auto it = std::ranges::find_if(patterns_, [pattern_id](const Pattern& p) {
        return p.id == pattern_id;
    });
    return it != patterns_.end() ? &(*it) : nullptr;
}

const Pattern* PatternLibrary::find_pattern(std::string_view pattern_id) const {
    auto it = std::ranges::find_if(patterns_, [pattern_id](const Pattern& p) {
        return p.id == pattern_id;
    });
    return it != patterns_.end() ? &(*it) : nullptr;
}

void PatternLibrary::clear() {
    patterns_.clear();
}

std::string PatternLibrary::duplicate_pattern(std::string_view pattern_id) {
    const auto* original = find_pattern(pattern_id);
    if (!original) {
        return "";
    }

    Pattern copy = *original;
    copy.id = generate_id();
    copy.name = original->name + " (Copy)";
    patterns_.push_back(copy);
    return copy.id;
}

std::string PatternLibrary::generate_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;

    std::stringstream ss;
    ss << "pat_" << std::hex << std::setfill('0');
    ss << std::setw(8) << dis(gen);
    return ss.str();
}

} // namespace furious
