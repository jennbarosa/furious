#include "furious/ui/patterns_window.hpp"
#include "furious/core/pattern_commands.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace furious {

void PatternsWindow::render() {
    if (!ImGui::Begin("Patterns")) {
        ImGui::End();
        return;
    }

    if (!library_) {
        ImGui::Text("No pattern library available");
        ImGui::End();
        return;
    }

    float available_width = ImGui::GetContentRegionAvail().x;
    float list_width = 200.0f;

    ImGui::BeginChild("PatternList", ImVec2(list_width, 0), true);
    render_pattern_list();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("PatternEditor", ImVec2(0, 0), true);
    render_pattern_editor();
    ImGui::EndChild();

    ImGui::End();
}

void PatternsWindow::render_pattern_list() {
    if (ImGui::Button("New Pattern")) {
        Pattern pattern;
        pattern.id = PatternLibrary::generate_id();
        pattern.name = "New Pattern";
        pattern.length_subdivisions = 16;

        if (execute_command_) {
            execute_command_(std::make_unique<AddPatternCommand>(*library_, pattern));
        }
        selected_pattern_id_ = pattern.id;
    }

    ImGui::Separator();

    ImGui::InputTextWithHint("##search", "Search...", search_buffer_, sizeof(search_buffer_));

    ImGui::Separator();

    for (const auto& pattern : library_->patterns()) {
        if (search_buffer_[0] != '\0') {
            bool found = false;
            const char* name = pattern.name.c_str();
            const char* filter = search_buffer_;
            for (size_t i = 0; name[i] != '\0'; ++i) {
                bool match = true;
                for (size_t j = 0; filter[j] != '\0'; ++j) {
                    if (std::tolower(static_cast<unsigned char>(name[i + j])) !=
                        std::tolower(static_cast<unsigned char>(filter[j]))) {
                        match = false;
                        break;
                    }
                }
                if (match) { found = true; break; }
            }
            if (!found) continue;
        }

        ImGui::PushID(pattern.id.c_str());

        bool is_selected = (pattern.id == selected_pattern_id_);

        if (ImGui::Selectable(pattern.name.c_str(), is_selected)) {
            selected_pattern_id_ = pattern.id;
            renaming_ = false;
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename")) {
                renaming_ = true;
                std::strncpy(rename_buffer_, pattern.name.c_str(), sizeof(rename_buffer_) - 1);
                rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
                selected_pattern_id_ = pattern.id;
            }
            if (ImGui::MenuItem("Duplicate")) {
                library_->duplicate_pattern(pattern.id);
            }
            if (ImGui::MenuItem("Delete")) {
                if (execute_command_) {
                    execute_command_(std::make_unique<RemovePatternCommand>(*library_, pattern.id));
                }
                if (selected_pattern_id_ == pattern.id) {
                    selected_pattern_id_.clear();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
}

void PatternsWindow::render_pattern_editor() {
    if (selected_pattern_id_.empty()) {
        ImGui::Text("Select a pattern to edit");
        return;
    }

    Pattern* pattern = library_->find_pattern(selected_pattern_id_);
    if (!pattern) {
        ImGui::Text("Pattern not found");
        return;
    }

    if (renaming_) {
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText("##rename", rename_buffer_, sizeof(rename_buffer_),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            Pattern old_state = *pattern;
            pattern->name = rename_buffer_;
            if (execute_command_) {
                execute_command_(std::make_unique<ModifyPatternCommand>(
                    *library_, pattern->id, old_state, *pattern, "Rename pattern"));
            }
            renaming_ = false;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            renaming_ = false;
        }
    } else {
        ImGui::Text("Pattern: %s", pattern->name.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Rename")) {
            renaming_ = true;
            std::strncpy(rename_buffer_, pattern->name.c_str(), sizeof(rename_buffer_) - 1);
            rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
        }
    }

    ImGui::Separator();

    ImGui::Text("Length (beats):");
    ImGui::SameLine();

    int current_beats = pattern->length_subdivisions / snap_subdivisions_per_beat_;
    int lengths[] = {1, 2, 4, 8, 16, 32};
    for (int len : lengths) {
        ImGui::SameLine();
        char label[16];
        std::snprintf(label, sizeof(label), "%d", len);
        if (ImGui::RadioButton(label, current_beats == len)) {
            begin_edit(*pattern);
            pattern->length_subdivisions = len * snap_subdivisions_per_beat_;
            end_edit(*pattern);
        }
    }

    ImGui::Text("Snap:");
    ImGui::SameLine();

    const char* snap_labels[] = {"1/4", "1/8", "1/16", "1/32"};
    int snap_values[] = {1, 2, 4, 8};
    for (int i = 0; i < 4; ++i) {
        ImGui::SameLine();
        if (ImGui::RadioButton(snap_labels[i], snap_subdivisions_per_beat_ == snap_values[i])) {
            int old_length_beats = pattern->length_subdivisions / snap_subdivisions_per_beat_;
            snap_subdivisions_per_beat_ = snap_values[i];
            begin_edit(*pattern);
            pattern->length_subdivisions = old_length_beats * snap_subdivisions_per_beat_;
            end_edit(*pattern);
        }
    }

    ImGui::Separator();

    ImGui::Text("Property:");
    ImGui::SameLine();

    auto prop_button = [this](PatternTargetProperty prop) {
        bool selected = (current_property_ == prop);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(property_name(prop))) {
            current_property_ = prop;
            selected_trigger_indices_.clear();
        }
        if (selected) {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
    };

    prop_button(PatternTargetProperty::PositionX);
    prop_button(PatternTargetProperty::PositionY);
    prop_button(PatternTargetProperty::ScaleX);
    prop_button(PatternTargetProperty::ScaleY);
    prop_button(PatternTargetProperty::Rotation);
    prop_button(PatternTargetProperty::FlipH);
    prop_button(PatternTargetProperty::FlipV);
    ImGui::NewLine();

    auto& prop_settings = pattern->settings_for(current_property_);
    bool restart_on_trigger = prop_settings.restart_on_trigger;
    if (ImGui::Checkbox("Restart clip on trigger", &restart_on_trigger)) {
        begin_edit(*pattern);
        pattern->settings_for(current_property_).restart_on_trigger = restart_on_trigger;
        end_edit(*pattern);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", prop_settings.restart_on_trigger ? "ON" : "OFF");

    ImGui::Text("Restart offset (beats):");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    float restart_offset = static_cast<float>(pattern->restart_offset_beats);
    if (ImGui::InputFloat("##restart_offset", &restart_offset, 0.25f, 1.0f, "%.2f")) {
        begin_edit(*pattern);
        pattern->restart_offset_beats = std::max(0.0, static_cast<double>(restart_offset));
        end_edit(*pattern);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(start position when clip restarts)");

    ImGui::Separator();

    render_grid(*pattern);

    ImGui::Separator();

    render_trigger_properties(*pattern);
}

void PatternsWindow::render_grid(Pattern& pattern) {
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size.y = 150.0f;
    last_canvas_width_ = canvas_size.x;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(30, 30, 35, 255)
    );

    float pixels_per_subdiv = BASE_PIXELS_PER_SUBDIVISION * zoom_;

    clamp_scroll(pattern);

    int start_subdiv = static_cast<int>(scroll_offset_ / pixels_per_subdiv);
    int end_subdiv = static_cast<int>((scroll_offset_ + canvas_size.x) / pixels_per_subdiv) + 1;
    start_subdiv = std::max(0, start_subdiv);
    end_subdiv = std::min(end_subdiv, pattern.length_subdivisions);

    for (int i = start_subdiv; i <= end_subdiv; ++i) {
        float x = canvas_pos.x + static_cast<float>(i) * pixels_per_subdiv - scroll_offset_;
        if (x < canvas_pos.x - 1.0f || x > canvas_pos.x + canvas_size.x + 1.0f) continue;

        bool is_beat = (i % snap_subdivisions_per_beat_ == 0);
        ImU32 color = is_beat ? IM_COL32(100, 100, 110, 255) : IM_COL32(50, 50, 55, 255);
        float thickness = is_beat ? 2.0f : 1.0f;
        draw_list->AddLine(
            ImVec2(x, canvas_pos.y),
            ImVec2(x, canvas_pos.y + canvas_size.y),
            color, thickness
        );
    }

    float center_y = canvas_pos.y + canvas_size.y / 2.0f;
    draw_list->AddLine(
        ImVec2(canvas_pos.x, center_y),
        ImVec2(canvas_pos.x + canvas_size.x, center_y),
        IM_COL32(60, 60, 70, 255), 1.0f
    );

    for (size_t i = 0; i < pattern.triggers.size(); ++i) {
        const auto& trigger = pattern.triggers[i];
        if (trigger.target != current_property_) continue;

        float x = canvas_pos.x + static_cast<float>(trigger.subdivision_index) * pixels_per_subdiv - scroll_offset_;
        if (x < canvas_pos.x - 10.0f || x > canvas_pos.x + canvas_size.x + 10.0f) continue;

        float normalized = normalize_value(trigger.value, current_property_);
        float y = canvas_pos.y + canvas_size.y * (1.0f - normalized);

        bool is_selected = selected_trigger_indices_.count(static_cast<int>(i)) > 0;
        ImU32 color = is_selected ? IM_COL32(255, 200, 100, 255) : IM_COL32(255, 150, 50, 255);

        draw_list->AddCircleFilled(ImVec2(x, y), 8.0f, color);
        draw_list->AddCircle(ImVec2(x, y), 8.0f, IM_COL32(255, 255, 255, 180), 0, 2.0f);
    }

    if (box_selecting_) {
        float drag_dist = std::abs(box_select_end_x_ - box_select_start_x_) +
                          std::abs(box_select_end_y_ - box_select_start_y_);
        if (drag_dist >= 5.0f) {
            float min_x = std::min(box_select_start_x_, box_select_end_x_);
            float max_x = std::max(box_select_start_x_, box_select_end_x_);
            float min_y = std::min(box_select_start_y_, box_select_end_y_);
            float max_y = std::max(box_select_start_y_, box_select_end_y_);

            draw_list->AddRectFilled(
                ImVec2(min_x, min_y),
                ImVec2(max_x, max_y),
                IM_COL32(255, 180, 100, 50)
            );
            draw_list->AddRect(
                ImVec2(min_x, min_y),
                ImVec2(max_x, max_y),
                IM_COL32(255, 180, 100, 200), 0.0f, 0, 1.0f
            );
        }
    }

    ImGui::InvisibleButton("grid", canvas_size);

    if (ImGui::IsItemHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            if (ImGui::GetIO().KeyCtrl) {
                float old_zoom = zoom_;
                zoom_ = std::clamp(zoom_ * (1.0f + wheel * 0.1f), 0.1f, 10.0f);

                ImVec2 mouse = ImGui::GetMousePos();
                float mouse_rel = mouse.x - canvas_pos.x;
                float subdiv_at_mouse = (scroll_offset_ + mouse_rel) / (BASE_PIXELS_PER_SUBDIVISION * old_zoom);
                scroll_offset_ = subdiv_at_mouse * BASE_PIXELS_PER_SUBDIVISION * zoom_ - mouse_rel;
                scroll_offset_ = std::max(0.0f, scroll_offset_);
            } else {
                scroll_offset_ = std::max(0.0f, scroll_offset_ - wheel * 50.0f);
            }
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ImVec2 mouse = ImGui::GetMousePos();
        float rel_x = mouse.x - canvas_pos.x + scroll_offset_;
        float rel_y = mouse.y - canvas_pos.y;

        int subdivision = static_cast<int>(rel_x / pixels_per_subdiv);
        subdivision = std::clamp(subdivision, 0, pattern.length_subdivisions - 1);

        int found_index = -1;
        for (size_t i = 0; i < pattern.triggers.size(); ++i) {
            const auto& t = pattern.triggers[i];
            if (t.target != current_property_) continue;

            float trigger_screen_x = canvas_pos.x + static_cast<float>(t.subdivision_index) * pixels_per_subdiv - scroll_offset_;
            float trigger_normalized = normalize_value(t.value, current_property_);
            float trigger_screen_y = canvas_pos.y + canvas_size.y * (1.0f - trigger_normalized);
            float dist_x = std::fabs(mouse.x - trigger_screen_x);
            float dist_y = std::fabs(mouse.y - trigger_screen_y);
            if (dist_x < 10.0f && dist_y < 10.0f) {
                found_index = static_cast<int>(i);
                break;
            }
        }

        if (found_index >= 0) {
            if (ImGui::GetIO().KeyCtrl) {
                if (selected_trigger_indices_.count(found_index)) {
                    selected_trigger_indices_.erase(found_index);
                } else {
                    selected_trigger_indices_.insert(found_index);
                }
            } else if (ImGui::GetIO().KeyShift && !selected_trigger_indices_.empty()) {
                int anchor = *selected_trigger_indices_.begin();
                int start = std::min(anchor, found_index);
                int end = std::max(anchor, found_index);
                for (int idx = start; idx <= end; ++idx) {
                    if (idx < static_cast<int>(pattern.triggers.size()) &&
                        pattern.triggers[idx].target == current_property_) {
                        selected_trigger_indices_.insert(idx);
                    }
                }
            } else {
                selected_trigger_indices_.clear();
                selected_trigger_indices_.insert(found_index);
            }
        } else {
            pending_had_selection_ = !selected_trigger_indices_.empty();
            box_selecting_ = true;
            box_select_start_x_ = mouse.x;
            box_select_start_y_ = mouse.y;
            box_select_end_x_ = mouse.x;
            box_select_end_y_ = mouse.y;

            float normalized = 1.0f - (rel_y / canvas_size.y);
            normalized = std::clamp(normalized, 0.0f, 1.0f);
            pending_click_subdivision_ = subdivision;
            pending_click_value_ = denormalize_value(normalized, current_property_);

            if (!ImGui::GetIO().KeyCtrl) {
                selected_trigger_indices_.clear();
            }
        }
    }

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse = ImGui::GetMousePos();

        if (box_selecting_) {
            box_select_end_x_ = mouse.x;
            box_select_end_y_ = mouse.y;

            float drag_dist = std::abs(box_select_end_x_ - box_select_start_x_) +
                              std::abs(box_select_end_y_ - box_select_start_y_);

            if (drag_dist >= 5.0f) {
                float min_x = std::min(box_select_start_x_, box_select_end_x_);
                float max_x = std::max(box_select_start_x_, box_select_end_x_);
                float min_y = std::min(box_select_start_y_, box_select_end_y_);
                float max_y = std::max(box_select_start_y_, box_select_end_y_);

                if (!ImGui::GetIO().KeyCtrl) {
                    selected_trigger_indices_.clear();
                }

                for (size_t i = 0; i < pattern.triggers.size(); ++i) {
                    const auto& t = pattern.triggers[i];
                    if (t.target != current_property_) continue;

                    float trigger_screen_x = canvas_pos.x + static_cast<float>(t.subdivision_index) * pixels_per_subdiv - scroll_offset_;
                    float trigger_normalized = normalize_value(t.value, current_property_);
                    float trigger_screen_y = canvas_pos.y + canvas_size.y * (1.0f - trigger_normalized);

                    if (trigger_screen_x >= min_x && trigger_screen_x <= max_x &&
                        trigger_screen_y >= min_y && trigger_screen_y <= max_y) {
                        selected_trigger_indices_.insert(static_cast<int>(i));
                    }
                }
            }
        } else if (selected_trigger_indices_.size() == 1) {
            int idx = *selected_trigger_indices_.begin();
            if (idx >= 0 && idx < static_cast<int>(pattern.triggers.size())) {
                auto& trigger = pattern.triggers[idx];
                if (trigger.target == current_property_) {
                    float rel_y = mouse.y - canvas_pos.y;
                    float normalized = 1.0f - (rel_y / canvas_size.y);
                    normalized = std::clamp(normalized, 0.0f, 1.0f);

                    if (!editing_) {
                        begin_edit(pattern);
                    }
                    trigger.value = denormalize_value(normalized, current_property_);
                }
            }
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (box_selecting_) {
            float drag_dist = std::abs(box_select_end_x_ - box_select_start_x_) +
                              std::abs(box_select_end_y_ - box_select_start_y_);

            if (drag_dist < 5.0f && pending_click_subdivision_ >= 0) {
                if (!pending_had_selection_) {
                    int existing_at_subdiv = -1;
                    for (size_t i = 0; i < pattern.triggers.size(); ++i) {
                        const auto& t = pattern.triggers[i];
                        if (t.target == current_property_ && t.subdivision_index == pending_click_subdivision_) {
                            existing_at_subdiv = static_cast<int>(i);
                            break;
                        }
                    }

                    if (existing_at_subdiv >= 0) {
                        selected_trigger_indices_.clear();
                        selected_trigger_indices_.insert(existing_at_subdiv);
                    } else {
                        begin_edit(pattern);
                        PatternTrigger trigger;
                        trigger.subdivision_index = pending_click_subdivision_;
                        trigger.target = current_property_;
                        trigger.value = pending_click_value_;
                        pattern.triggers.push_back(trigger);
                        selected_trigger_indices_.clear();
                        selected_trigger_indices_.insert(static_cast<int>(pattern.triggers.size()) - 1);
                        end_edit(pattern);
                    }
                }
            }

            box_selecting_ = false;
            pending_click_subdivision_ = -1;
        }
        if (editing_) {
            end_edit(pattern);
        }
    }
}

void PatternsWindow::clamp_scroll(const Pattern& pattern) {
    float pixels_per_subdiv = BASE_PIXELS_PER_SUBDIVISION * zoom_;
    float max_scroll = pattern.length_subdivisions * pixels_per_subdiv - last_canvas_width_;
    scroll_offset_ = std::clamp(scroll_offset_, 0.0f, std::max(0.0f, max_scroll));
}

void PatternsWindow::render_trigger_properties(Pattern& pattern) {
    if (selected_trigger_indices_.empty()) {
        ImGui::Text("Click on the grid to add or select a trigger");
        return;
    }

    if (selected_trigger_indices_.size() > 1) {
        ImGui::Text("%zu triggers selected", selected_trigger_indices_.size());

        ImGui::Separator();
        ImGui::Text("Bulk Duration");

        bool any_custom = false;
        bool all_custom = true;
        int common_duration = -1;
        bool same_duration = true;

        for (int idx : selected_trigger_indices_) {
            if (idx >= 0 && idx < static_cast<int>(pattern.triggers.size())) {
                const auto& t = pattern.triggers[idx];
                if (t.duration_subdivisions > 0) {
                    any_custom = true;
                    if (common_duration < 0) {
                        common_duration = t.duration_subdivisions;
                    } else if (common_duration != t.duration_subdivisions) {
                        same_duration = false;
                    }
                } else {
                    all_custom = false;
                }
            }
        }

        bool use_custom = all_custom;
        if (ImGui::Checkbox("Custom Duration", &use_custom)) {
            begin_edit(pattern);
            for (int idx : selected_trigger_indices_) {
                if (idx >= 0 && idx < static_cast<int>(pattern.triggers.size())) {
                    pattern.triggers[idx].duration_subdivisions = use_custom ? 4 : -1;
                }
            }
            end_edit(pattern);
        }
        if (any_custom && !all_custom) {
            ImGui::SameLine();
            ImGui::TextDisabled("(mixed)");
        }

        if (any_custom && tempo_) {
            double subdivs_per_beat = 4.0;
            double ms_per_subdiv = (tempo_->beat_duration_seconds() / subdivs_per_beat) * 1000.0;
            int max_ms = static_cast<int>(pattern.length_subdivisions * ms_per_subdiv);
            int current_ms = same_duration && common_duration > 0
                ? static_cast<int>(common_duration * ms_per_subdiv)
                : static_cast<int>(4 * ms_per_subdiv);

            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::SliderInt("ms##bulk_dur_ms", &current_ms, 1, max_ms)) {
                if (!editing_) begin_edit(pattern);
                int new_subdivs = std::max(1, static_cast<int>(std::round(current_ms / ms_per_subdiv)));
                new_subdivs = std::min(new_subdivs, pattern.length_subdivisions);
                for (int idx : selected_trigger_indices_) {
                    if (idx >= 0 && idx < static_cast<int>(pattern.triggers.size())) {
                        if (pattern.triggers[idx].duration_subdivisions > 0 || use_custom) {
                            pattern.triggers[idx].duration_subdivisions = new_subdivs;
                        }
                    }
                }
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                end_edit(pattern);
            }
            if (!same_duration) {
                ImGui::SameLine();
                ImGui::TextDisabled("(mixed)");
            }

            int quarter_subdivs = static_cast<int>(subdivs_per_beat);
            int eighth_subdivs = static_cast<int>(subdivs_per_beat / 2);
            int sixteenth_subdivs = static_cast<int>(subdivs_per_beat / 4);

            ImGui::Text("Presets:");
            ImGui::SameLine();
            if (quarter_subdivs <= pattern.length_subdivisions) {
                if (ImGui::SmallButton("1/4##bulk")) {
                    begin_edit(pattern);
                    for (int idx : selected_trigger_indices_) {
                        if (idx >= 0 && idx < static_cast<int>(pattern.triggers.size())) {
                            pattern.triggers[idx].duration_subdivisions = quarter_subdivs;
                        }
                    }
                    end_edit(pattern);
                }
                ImGui::SameLine();
            }
            if (eighth_subdivs >= 1 && eighth_subdivs <= pattern.length_subdivisions) {
                if (ImGui::SmallButton("1/8##bulk")) {
                    begin_edit(pattern);
                    for (int idx : selected_trigger_indices_) {
                        if (idx >= 0 && idx < static_cast<int>(pattern.triggers.size())) {
                            pattern.triggers[idx].duration_subdivisions = eighth_subdivs;
                        }
                    }
                    end_edit(pattern);
                }
                ImGui::SameLine();
            }
            if (sixteenth_subdivs >= 1 && sixteenth_subdivs <= pattern.length_subdivisions) {
                if (ImGui::SmallButton("1/16##bulk")) {
                    begin_edit(pattern);
                    for (int idx : selected_trigger_indices_) {
                        if (idx >= 0 && idx < static_cast<int>(pattern.triggers.size())) {
                            pattern.triggers[idx].duration_subdivisions = sixteenth_subdivs;
                        }
                    }
                    end_edit(pattern);
                }
                ImGui::SameLine();
            }
            if (ImGui::SmallButton("Clear All##bulk")) {
                begin_edit(pattern);
                for (int idx : selected_trigger_indices_) {
                    if (idx >= 0 && idx < static_cast<int>(pattern.triggers.size())) {
                        pattern.triggers[idx].duration_subdivisions = -1;
                    }
                }
                end_edit(pattern);
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Delete Selected")) {
            begin_edit(pattern);
            std::vector<int> to_delete(selected_trigger_indices_.begin(), selected_trigger_indices_.end());
            std::sort(to_delete.rbegin(), to_delete.rend());
            for (int idx : to_delete) {
                if (idx < static_cast<int>(pattern.triggers.size())) {
                    pattern.triggers.erase(pattern.triggers.begin() + idx);
                }
            }
            selected_trigger_indices_.clear();
            end_edit(pattern);
        }
        return;
    }

    int selected_idx = *selected_trigger_indices_.begin();
    if (selected_idx < 0 || selected_idx >= static_cast<int>(pattern.triggers.size())) {
        ImGui::Text("Invalid selection");
        return;
    }

    auto& trigger = pattern.triggers[selected_idx];

    ImGui::Text("Trigger at subdivision %d", trigger.subdivision_index);

    if (ImGui::DragFloat("Value", &trigger.value, 0.01f)) {
        if (!editing_) {
            begin_edit(pattern);
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        end_edit(pattern);
    }

    ImGui::Separator();
    ImGui::Text("Duration");

    int next_trigger_subdiv = -1;
    for (const auto& t : pattern.triggers) {
        if (t.subdivision_index > trigger.subdivision_index) {
            if (next_trigger_subdiv < 0 || t.subdivision_index < next_trigger_subdiv) {
                next_trigger_subdiv = t.subdivision_index;
            }
        }
    }
    if (next_trigger_subdiv < 0) {
        for (const auto& t : pattern.triggers) {
            if (t.subdivision_index < trigger.subdivision_index) {
                if (next_trigger_subdiv < 0 || t.subdivision_index < next_trigger_subdiv) {
                    next_trigger_subdiv = t.subdivision_index;
                }
            }
        }
        if (next_trigger_subdiv >= 0) {
            next_trigger_subdiv += pattern.length_subdivisions;
        }
    }

    int max_duration = (next_trigger_subdiv >= 0)
        ? (next_trigger_subdiv - trigger.subdivision_index)
        : pattern.length_subdivisions;
    if (max_duration <= 0) max_duration = pattern.length_subdivisions;

    bool use_custom = trigger.duration_subdivisions > 0;
    if (ImGui::Checkbox("Custom Duration", &use_custom)) {
        if (!editing_) begin_edit(pattern);
        trigger.duration_subdivisions = use_custom ? std::min(4, max_duration) : -1;
        end_edit(pattern);
    }

    if (use_custom && tempo_) {
        double subdivs_per_beat = 4.0;
        double ms_per_subdiv = (tempo_->beat_duration_seconds() / subdivs_per_beat) * 1000.0;
        int max_ms = static_cast<int>(max_duration * ms_per_subdiv);
        int current_ms = static_cast<int>(trigger.duration_subdivisions * ms_per_subdiv);

        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::SliderInt("ms##dur_ms", &current_ms, 1, max_ms)) {
            if (!editing_) begin_edit(pattern);
            int new_subdivs = std::max(1, static_cast<int>(std::round(current_ms / ms_per_subdiv)));
            new_subdivs = std::min(new_subdivs, max_duration);
            trigger.duration_subdivisions = new_subdivs;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            end_edit(pattern);
        }

        ImGui::SameLine();
        ImGui::TextDisabled("(%d subdivs)", trigger.duration_subdivisions);

        int quarter_subdivs = static_cast<int>(subdivs_per_beat);
        int eighth_subdivs = static_cast<int>(subdivs_per_beat / 2);
        int sixteenth_subdivs = static_cast<int>(subdivs_per_beat / 4);

        ImGui::Text("Presets:");
        ImGui::SameLine();
        if (quarter_subdivs <= max_duration) {
            if (ImGui::SmallButton("1/4##single")) {
                if (!editing_) begin_edit(pattern);
                trigger.duration_subdivisions = quarter_subdivs;
                end_edit(pattern);
            }
            ImGui::SameLine();
        }
        if (eighth_subdivs >= 1 && eighth_subdivs <= max_duration) {
            if (ImGui::SmallButton("1/8##single")) {
                if (!editing_) begin_edit(pattern);
                trigger.duration_subdivisions = eighth_subdivs;
                end_edit(pattern);
            }
            ImGui::SameLine();
        }
        if (sixteenth_subdivs >= 1 && sixteenth_subdivs <= max_duration) {
            if (ImGui::SmallButton("1/16##single")) {
                if (!editing_) begin_edit(pattern);
                trigger.duration_subdivisions = sixteenth_subdivs;
                end_edit(pattern);
            }
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Delete Trigger")) {
        begin_edit(pattern);
        pattern.triggers.erase(pattern.triggers.begin() + selected_idx);
        selected_trigger_indices_.clear();
        end_edit(pattern);
    }
}

void PatternsWindow::begin_edit(const Pattern& pattern) {
    if (!editing_) {
        edit_initial_state_ = pattern;
        editing_ = true;
    }
}

void PatternsWindow::end_edit(Pattern& pattern) {
    if (editing_ && edit_initial_state_ && execute_command_) {
        execute_command_(std::make_unique<ModifyPatternCommand>(
            *library_, pattern.id, *edit_initial_state_, pattern, "Edit pattern"));
    }
    editing_ = false;
    edit_initial_state_.reset();
}

float PatternsWindow::normalize_value(float value, PatternTargetProperty prop) const {
    switch (prop) {
        case PatternTargetProperty::PositionX:
        case PatternTargetProperty::PositionY:
            return (value + 500.0f) / 1000.0f;
        case PatternTargetProperty::ScaleX:
        case PatternTargetProperty::ScaleY:
            return value / 3.0f;
        case PatternTargetProperty::Rotation:
            return (value + 180.0f) / 360.0f;
        case PatternTargetProperty::FlipH:
        case PatternTargetProperty::FlipV:
            return value != 0.0f ? 0.75f : 0.25f;
    }
    return 0.5f;
}

float PatternsWindow::denormalize_value(float normalized, PatternTargetProperty prop) const {
    switch (prop) {
        case PatternTargetProperty::PositionX:
        case PatternTargetProperty::PositionY:
            return normalized * 1000.0f - 500.0f;
        case PatternTargetProperty::ScaleX:
        case PatternTargetProperty::ScaleY:
            return normalized * 3.0f;
        case PatternTargetProperty::Rotation:
            return normalized * 360.0f - 180.0f;
        case PatternTargetProperty::FlipH:
        case PatternTargetProperty::FlipV:
            return normalized >= 0.5f ? 1.0f : 0.0f;
    }
    return 0.0f;
}

const char* PatternsWindow::property_name(PatternTargetProperty prop) const {
    switch (prop) {
        case PatternTargetProperty::PositionX: return "Pos X";
        case PatternTargetProperty::PositionY: return "Pos Y";
        case PatternTargetProperty::ScaleX: return "Scale X";
        case PatternTargetProperty::ScaleY: return "Scale Y";
        case PatternTargetProperty::Rotation: return "Rotation";
        case PatternTargetProperty::FlipH: return "Flip H";
        case PatternTargetProperty::FlipV: return "Flip V";
    }
    return "Unknown";
}

} // namespace furious
