#include "furious/ui/pitch_editor_window.hpp"
#include "furious/audio/pitch_estimator.hpp"
#include "furious/core/pitch_commands.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace furious {

void PitchEditorWindow::render() {
    if (!open_) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Pitch Editor", &open_)) {
        ImGui::End();
        return;
    }

    if (!library_) {
        ImGui::Text("No pitch library available");
        ImGui::End();
        return;
    }

    float list_width = 200.0f;

    ImGui::BeginChild("TrackList", ImVec2(list_width, 0), true);
    render_track_list();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("TrackEditor", ImVec2(0, 0), true);
    render_track_editor();
    ImGui::EndChild();

    ImGui::End();
}

void PitchEditorWindow::render_track_list() {
    if (ImGui::Button("New Track")) {
        std::string id = library_->create_track("New Pitch Track");
        selected_track_id_ = id;
    }

    ImGui::Separator();

    ImGui::InputTextWithHint("##search", "Search...", search_buffer_, sizeof(search_buffer_));

    ImGui::Separator();

    for (const auto& track : library_->tracks()) {
        if (search_buffer_[0] != '\0') {
            bool found = false;
            const char* name = track.name.c_str();
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

        ImGui::PushID(track.id.c_str());

        bool is_selected = (track.id == selected_track_id_);

        if (ImGui::Selectable(track.name.c_str(), is_selected)) {
            selected_track_id_ = track.id;
            renaming_ = false;
            selected_note_indices_.clear();
        }

        if (ImGui::BeginPopupContextItem()) {
            std::string context_track_id = track.id;
            std::string context_track_name = track.name;
            if (ImGui::MenuItem("Rename")) {
                renaming_ = true;
                std::strncpy(rename_buffer_, context_track_name.c_str(), sizeof(rename_buffer_) - 1);
                rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
                selected_track_id_ = context_track_id;
            }
            if (ImGui::MenuItem("Duplicate")) {
                std::string new_id = library_->duplicate_track(context_track_id);
                if (!new_id.empty()) {
                    selected_track_id_ = new_id;
                    selected_note_indices_.clear();
                }
            }
            if (ImGui::MenuItem("Delete")) {
                library_->remove_track(context_track_id);
                if (selected_track_id_ == context_track_id) {
                    selected_track_id_.clear();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
}

void PitchEditorWindow::render_track_editor() {
    if (selected_track_id_.empty()) {
        ImGui::Text("Select a track to edit");
        return;
    }

    PitchTrack* track = library_->find_track(selected_track_id_);
    if (!track) {
        ImGui::Text("Track not found");
        return;
    }

    if (renaming_) {
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText("##rename", rename_buffer_, sizeof(rename_buffer_),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            track->name = rename_buffer_;
            renaming_ = false;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            renaming_ = false;
        }
    } else {
        ImGui::Text("Track: %s", track->name.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Rename")) {
            renaming_ = true;
            std::strncpy(rename_buffer_, track->name.c_str(), sizeof(rename_buffer_) - 1);
            rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
        }
    }

    ImGui::Separator();

    ImGui::Text("Length (beats):");
    ImGui::SameLine();

    int current_beats = track->length_subdivisions / subdivisions_per_beat_;
    int lengths[] = {1, 2, 4, 8, 16, 32};
    for (int len : lengths) {
        ImGui::SameLine();
        char label[16];
        std::snprintf(label, sizeof(label), "%d", len);
        if (ImGui::RadioButton(label, current_beats == len)) {
            begin_edit(*track);
            track->length_subdivisions = len * subdivisions_per_beat_;
            end_edit(*track);
        }
    }

    ImGui::Text("Snap:");
    ImGui::SameLine();
    const char* snap_labels[] = {"1/4", "1/8", "1/16", "1/32"};
    int snap_values[] = {1, 2, 4, 8};
    for (int i = 0; i < 4; ++i) {
        ImGui::SameLine();
        if (ImGui::RadioButton(snap_labels[i], subdivisions_per_beat_ == snap_values[i])) {
            int old_length_beats = track->length_subdivisions / subdivisions_per_beat_;
            subdivisions_per_beat_ = snap_values[i];
            begin_edit(*track);
            track->length_subdivisions = old_length_beats * subdivisions_per_beat_;
            end_edit(*track);
        }
    }

    ImGui::Separator();

    if (audio_clip_ && audio_clip_->is_loaded()) {
        if (ImGui::Button("Estimate from BGM")) {
            estimate_from_bgm(*track);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(Analyzes loaded audio)");
    } else {
        ImGui::TextDisabled("Load audio to enable pitch estimation");
    }

    ImGui::Checkbox("Preview audio on note placement", &preview_enabled_);
    if (preview_enabled_) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::SliderFloat("##preview_duration", &preview_duration_, 0.1f, 2.0f, "%.1fs");

        ImGui::Text("BGM Vol:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        int bgm_pct = static_cast<int>(bgm_volume_ * 100.0f);
        if (ImGui::SliderInt("##bgm_vol", &bgm_pct, 0, 100, "%d%%")) {
            bgm_volume_ = static_cast<float>(bgm_pct) / 100.0f;
        }
        ImGui::SameLine();
        ImGui::Text("Clip Vol:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        int clip_pct = static_cast<int>(clip_volume_ * 100.0f);
        if (ImGui::SliderInt("##clip_vol", &clip_pct, 0, 100, "%d%%")) {
            clip_volume_ = static_cast<float>(clip_pct) / 100.0f;
        }
    }

    ImGui::Checkbox("Autotune Enabled", &track->autotune_enabled);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When enabled, clips using this track will be pitch-corrected to match the notes");
    }

    if (track->autotune_enabled) {
        ImGui::Text("Pitch Smoothing:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        int smoothing_pct = static_cast<int>(track->smoothing * 100.0f);
        if (ImGui::SliderInt("##smoothing", &smoothing_pct, 0, 100, "%d%%")) {
            track->smoothing = static_cast<float>(smoothing_pct) / 100.0f;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Controls how quickly pitch changes are applied.\n0%% = instant, 100%% = very slow/smooth");
        }
    }

    if (pattern_library_) {
        ImGui::Text("Pattern Overlay:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);

        const char* current_name = overlay_pattern_id_.empty() ? "None" : nullptr;
        if (!current_name) {
            const Pattern* p = pattern_library_->find_pattern(overlay_pattern_id_);
            current_name = p ? p->name.c_str() : "None";
        }

        if (ImGui::BeginCombo("##pattern_overlay", current_name)) {
            if (ImGui::Selectable("None", overlay_pattern_id_.empty())) {
                overlay_pattern_id_.clear();
            }
            for (const auto& pattern : pattern_library_->patterns()) {
                bool is_selected = (pattern.id == overlay_pattern_id_);
                if (ImGui::Selectable(pattern.name.c_str(), is_selected)) {
                    overlay_pattern_id_ = pattern.id;
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Separator();

    float properties_height = 130.0f;
    float available_height = ImGui::GetContentRegionAvail().y;
    float grid_height = std::max(200.0f, available_height - properties_height - 10.0f);

    ImGui::BeginChild("GridArea", ImVec2(0, grid_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    render_grid(*track);
    ImGui::EndChild();

    ImGui::Separator();

    render_note_properties(*track);
}

void PitchEditorWindow::render_grid(PitchTrack& track) {
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    last_canvas_width_ = canvas_size.x;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(
        canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(30, 30, 35, 255)
    );

    draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);

    float pixels_per_subdiv = BASE_PIXELS_PER_SUBDIVISION * zoom_;

    clamp_scroll(track);

    float note_height = (canvas_size.y * vertical_zoom_) / static_cast<float>(NOTE_RANGE);
    for (int midi = MIN_MIDI_NOTE; midi <= MAX_MIDI_NOTE; ++midi) {
        float y = midi_to_y(midi, canvas_pos.y, canvas_size.y);

        if (y < canvas_pos.y - 1.0f || y > canvas_pos.y + canvas_size.y + 1.0f) continue;

        bool is_c = (midi % 12 == 0);
        ImU32 color = is_c ? IM_COL32(80, 80, 90, 255) : IM_COL32(45, 45, 50, 255);

        draw_list->AddRectFilled(
            ImVec2(canvas_pos.x, y),
            ImVec2(canvas_pos.x + canvas_size.x, y + 1.0f),
            color
        );

        if (is_c) {
            draw_list->AddText(ImVec2(canvas_pos.x + 2, y - 8), IM_COL32(150, 150, 150, 255), midi_to_note_name(midi).c_str());
        }
    }

    int start_subdiv = static_cast<int>(scroll_offset_ / pixels_per_subdiv);
    int end_subdiv = static_cast<int>((scroll_offset_ + canvas_size.x) / pixels_per_subdiv) + 1;
    start_subdiv = std::max(0, start_subdiv);
    end_subdiv = std::min(end_subdiv, track.length_subdivisions);

    bool draw_subdivisions = pixels_per_subdiv >= 8.0f;

    for (int i = start_subdiv; i <= end_subdiv; ++i) {
        bool is_beat = (i % subdivisions_per_beat_ == 0);
        if (!is_beat && !draw_subdivisions) continue;

        float x = canvas_pos.x + static_cast<float>(i) * pixels_per_subdiv - scroll_offset_;
        if (x < canvas_pos.x - 1.0f || x > canvas_pos.x + canvas_size.x + 1.0f) continue;

        ImU32 color = is_beat ? IM_COL32(100, 100, 110, 255) : IM_COL32(50, 50, 55, 255);
        draw_list->AddRectFilled(
            ImVec2(x, canvas_pos.y),
            ImVec2(x + 1.0f, canvas_pos.y + canvas_size.y),
            color
        );
    }

    if (pattern_library_ && !overlay_pattern_id_.empty()) {
        const Pattern* overlay_pattern = pattern_library_->find_pattern(overlay_pattern_id_);
        if (overlay_pattern) {
            for (const auto& trigger : overlay_pattern->triggers) {
                int subdiv = trigger.subdivision_index;
                if (subdiv < start_subdiv || subdiv > end_subdiv) continue;

                float x = canvas_pos.x + static_cast<float>(subdiv) * pixels_per_subdiv - scroll_offset_;
                if (x < canvas_pos.x - 1.0f || x > canvas_pos.x + canvas_size.x + 1.0f) continue;

                draw_list->AddRectFilled(
                    ImVec2(x, canvas_pos.y),
                    ImVec2(x + pixels_per_subdiv * 0.8f, canvas_pos.y + canvas_size.y),
                    IM_COL32(255, 180, 50, 40)
                );

                draw_list->AddRectFilled(
                    ImVec2(x, canvas_pos.y),
                    ImVec2(x + 2.0f, canvas_pos.y + canvas_size.y),
                    IM_COL32(255, 180, 50, 150)
                );
            }
        }
    }

    for (size_t i = 0; i < track.notes.size(); ++i) {
        const auto& note = track.notes[i];

        float x = canvas_pos.x + static_cast<float>(note.subdivision_index) * pixels_per_subdiv - scroll_offset_;
        if (x < canvas_pos.x - 15.0f || x > canvas_pos.x + canvas_size.x + 15.0f) continue;

        float y = midi_to_y(note.midi_note, canvas_pos.y, canvas_size.y);

        bool is_selected = selected_note_indices_.count(static_cast<int>(i)) > 0;
        ImU32 color = is_selected ? IM_COL32(100, 200, 255, 255) : IM_COL32(50, 150, 255, 255);

        float note_width = pixels_per_subdiv * 0.8f;
        draw_list->AddRectFilled(
            ImVec2(x, y - note_height * 0.4f),
            ImVec2(x + note_width, y + note_height * 0.4f),
            color
        );

        if (is_selected) {
            draw_list->AddRect(
                ImVec2(x - 1, y - note_height * 0.4f - 1),
                ImVec2(x + note_width + 1, y + note_height * 0.4f + 1),
                IM_COL32(255, 255, 255, 200)
            );
        }

        std::string name = note.note_name();
        ImVec2 text_size = ImGui::CalcTextSize(name.c_str());
        if (text_size.x < note_width - 4) {
            draw_list->AddText(
                ImVec2(x + 2, y - text_size.y * 0.5f),
                IM_COL32(255, 255, 255, 255), name.c_str()
            );
        }
    }

    if (box_selecting_) {
        float min_x = std::min(box_select_start_x_, box_select_end_x_);
        float max_x = std::max(box_select_start_x_, box_select_end_x_);
        float min_y = std::min(box_select_start_y_, box_select_end_y_);
        float max_y = std::max(box_select_start_y_, box_select_end_y_);

        draw_list->AddRectFilled(
            ImVec2(min_x, min_y),
            ImVec2(max_x, max_y),
            IM_COL32(100, 150, 255, 50)
        );
        draw_list->AddRect(
            ImVec2(min_x, min_y),
            ImVec2(max_x, max_y),
            IM_COL32(100, 150, 255, 200), 0.0f, 0, 1.0f
        );
    }

    float playhead_subdivision = static_cast<float>(playhead_beat_ * subdivisions_per_beat_);
    float track_length_subdivs = static_cast<float>(track.length_subdivisions);
    if (track_length_subdivs > 0.0f) {
        float wrapped_subdivision = std::fmod(playhead_subdivision, track_length_subdivs);
        if (wrapped_subdivision < 0.0f) {
            wrapped_subdivision += track_length_subdivs;
        }
        float playhead_x = canvas_pos.x + wrapped_subdivision * pixels_per_subdiv - scroll_offset_;
        if (playhead_x >= canvas_pos.x && playhead_x <= canvas_pos.x + canvas_size.x) {
            draw_list->AddLine(
                ImVec2(playhead_x, canvas_pos.y),
                ImVec2(playhead_x, canvas_pos.y + canvas_size.y),
                IM_COL32(255, 80, 80, 255), 2.0f
            );
        }
    }

    draw_list->PopClipRect();

    ImGui::InvisibleButton("pitch_grid", canvas_size);
}
void PitchEditorWindow::render_note_properties(PitchTrack& /*track*/) {}
void PitchEditorWindow::estimate_from_bgm(PitchTrack& /*track*/) {}
void PitchEditorWindow::handle_keyboard_shortcuts(PitchTrack& /*track*/) {}
void PitchEditorWindow::select_all_notes(const PitchTrack& /*track*/) {}
void PitchEditorWindow::shift_selected_notes(PitchTrack& /*track*/, int /*semitones*/) {}
void PitchEditorWindow::clamp_scroll(const PitchTrack& /*track*/) {}
void PitchEditorWindow::trigger_preview(const PitchTrack& /*track*/, int /*subdivision*/) {}

void PitchEditorWindow::begin_edit(const PitchTrack& track) {
    if (!editing_) {
        edit_initial_state_ = track;
        editing_ = true;
    }
}

void PitchEditorWindow::end_edit(PitchTrack& track) {
    if (editing_ && edit_initial_state_ && execute_command_) {
        execute_command_(std::make_unique<ModifyPitchTrackCommand>(
            *library_, track.id, *edit_initial_state_, track, "Edit pitch track"));
    }
    editing_ = false;
    edit_initial_state_.reset();
}

float PitchEditorWindow::midi_to_y(int midi_note, float canvas_y, float canvas_height) const {
    float zoomed_height = canvas_height * vertical_zoom_;
    float normalized = static_cast<float>(midi_note - MIN_MIDI_NOTE) / static_cast<float>(NOTE_RANGE);
    float center_offset = (zoomed_height - canvas_height) * 0.5f;
    return canvas_y - center_offset + zoomed_height * (1.0f - normalized);
}

int PitchEditorWindow::y_to_midi(float y, float canvas_y, float canvas_height) const {
    float zoomed_height = canvas_height * vertical_zoom_;
    float center_offset = (zoomed_height - canvas_height) * 0.5f;
    float normalized = 1.0f - (y - canvas_y + center_offset) / zoomed_height;
    return MIN_MIDI_NOTE + static_cast<int>(normalized * NOTE_RANGE);
}

bool PitchEditorWindow::consume_play_toggle_request() {
    bool result = play_toggle_requested_;
    play_toggle_requested_ = false;
    return result;
}

} // namespace furious
