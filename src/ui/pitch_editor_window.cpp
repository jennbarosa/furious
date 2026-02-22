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

void PitchEditorWindow::render_grid(PitchTrack& /*track*/) {}
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
