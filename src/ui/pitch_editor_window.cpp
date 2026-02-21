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
    ImGui::Text("Select a track to edit");
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
