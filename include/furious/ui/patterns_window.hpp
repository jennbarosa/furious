#pragma once

#include "furious/core/command.hpp"
#include "furious/core/pattern.hpp"
#include "furious/core/pattern_library.hpp"
#include "furious/core/tempo.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace furious {

class PatternsWindow {
public:
    PatternsWindow() = default;

    void render();

    void set_pattern_library(PatternLibrary* library) { library_ = library; }
    void set_tempo(const Tempo* tempo) { tempo_ = tempo; }
    void set_command_callback(std::function<void(std::unique_ptr<Command>)> callback) {
        execute_command_ = std::move(callback);
    }

    [[nodiscard]] const std::string& selected_pattern_id() const { return selected_pattern_id_; }

private:
    PatternLibrary* library_ = nullptr;
    const Tempo* tempo_ = nullptr;
    std::function<void(std::unique_ptr<Command>)> execute_command_;

    std::string selected_pattern_id_;
    std::string search_filter_;
    char search_buffer_[256] = {};
    char rename_buffer_[256] = {};
    bool renaming_ = false;

    PatternTargetProperty current_property_ = PatternTargetProperty::ScaleX;
    float current_value_ = 1.0f;
    int selected_trigger_index_ = -1;
    int snap_subdivisions_per_beat_ = 4;

    float zoom_ = 1.0f;
    float scroll_offset_ = 0.0f;
    float last_canvas_width_ = 0.0f;
    static constexpr float BASE_PIXELS_PER_SUBDIVISION = 20.0f;

    std::optional<Pattern> edit_initial_state_;
    bool editing_ = false;

    void clamp_scroll(const Pattern& pattern);

    void render_pattern_list();
    void render_pattern_editor();
    void render_grid(Pattern& pattern);
    void render_trigger_properties(Pattern& pattern);

    void begin_edit(const Pattern& pattern);
    void end_edit(Pattern& pattern);

    [[nodiscard]] float normalize_value(float value, PatternTargetProperty prop) const;
    [[nodiscard]] float denormalize_value(float normalized, PatternTargetProperty prop) const;
    [[nodiscard]] const char* property_name(PatternTargetProperty prop) const;
};

} // namespace furious
