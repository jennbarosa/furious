#pragma once

#include <magic_enum/magic_enum.hpp>
#include <string>
#include <cctype>

namespace furious {

template<typename E>
std::string enum_to_string(E value) {
    auto name = magic_enum::enum_name(value);
    std::string result;
    result.reserve(name.size());
    for (char c : name) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

template<typename E>
E string_to_enum(const std::string& str, E default_value) {
    auto result = magic_enum::enum_cast<E>(str, magic_enum::case_insensitive);
    return result.value_or(default_value);
}

} // namespace furious
