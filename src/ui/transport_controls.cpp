#include "furious/ui/transport_controls.hpp"
#include "imgui.h"
#include <cmath>
#include <nfd.h>
#include <string>

namespace furious {

TransportControls::TransportControls(Project& project) : project_(project) {}

void TransportControls::render() {
    ImGui::Begin("Project");

    if (ImGui::Button(is_playing_ ? "Stop" : "Play")) {
        is_playing_ = !is_playing_;
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        is_playing_ = false;
        reset_requested_ = true;
    }

    ImGui::Separator();

    float bpm = static_cast<float>(project_.tempo().bpm());
    if (ImGui::DragFloat("BPM", &bpm, 1.0f, 1.0f, 999.0f, "%.1f")) {
        project_.tempo().set_bpm(bpm);
    }

    ImGui::Separator();

    const char* subdivisions[] = {"1/4", "1/8", "1/16"};
    int current = 0;
    switch (project_.grid_subdivision()) {
        case NoteSubdivision::Quarter: current = 0; break;
        case NoteSubdivision::Eighth: current = 1; break;
        case NoteSubdivision::Sixteenth: current = 2; break;
    }

    if (ImGui::Combo("Grid", &current, subdivisions, 3)) {
        switch (current) {
            case 0: project_.set_grid_subdivision(NoteSubdivision::Quarter); break;
            case 1: project_.set_grid_subdivision(NoteSubdivision::Eighth); break;
            case 2: project_.set_grid_subdivision(NoteSubdivision::Sixteenth); break;
        }
    }

    ImGui::Separator();

    const char* fps_options[] = {"24", "25", "30", "50", "60"};
    const double fps_values[] = {24.0, 25.0, 30.0, 50.0, 60.0};
    int fps_current = 2;
    double current_fps = project_.fps();
    for (int i = 0; i < 5; ++i) {
        if (std::abs(current_fps - fps_values[i]) < 0.1) {
            fps_current = i;
            break;
        }
    }

    if (ImGui::Combo("FPS", &fps_current, fps_options, 5)) {
        project_.set_fps(fps_values[fps_current]);
    }

    ImGui::Separator();

    bool snap = project_.snap_enabled();
    if (ImGui::Checkbox("Snap to Grid", &snap)) {
        project_.set_snap_enabled(snap);
    }
    ImGui::Checkbox("Metronome", &metronome_enabled_);
    ImGui::Checkbox("Follow Playhead", &follow_playhead_);
    ImGui::Checkbox("Loop", &loop_enabled_);

    ImGui::Separator();

    if (ImGui::Button("Save")) {
        if (!current_project_path_.empty()) {
            requested_filepath_ = current_project_path_;
            save_requested_ = true;
        } else {
            nfdu8char_t* out_path = nullptr;
            nfdu8filteritem_t filters[] = {{"FURIOUS Project", "furious"}};
            nfdsavedialogu8args_t args = {0};
            args.filterList = filters;
            args.filterCount = 1;
            std::string default_name = project_.name() + ".furious";
            args.defaultName = default_name.c_str();

            if (NFD_SaveDialogU8_With(&out_path, &args) == NFD_OKAY) {
                requested_filepath_ = out_path;
                NFD_FreePathU8(out_path);
                save_requested_ = true;
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As")) {
        nfdu8char_t* out_path = nullptr;
        nfdu8filteritem_t filters[] = {{"FURIOUS Project", "furious"}};
        nfdsavedialogu8args_t args = {0};
        args.filterList = filters;
        args.filterCount = 1;

        std::string default_name = project_.name() + ".furious";
        if (!current_project_path_.empty()) {
            size_t last_slash = current_project_path_.find_last_of("/\\");
            if (last_slash != std::string::npos) {
                default_name = current_project_path_.substr(last_slash + 1);
            } else {
                default_name = current_project_path_;
            }
        }
        args.defaultName = default_name.c_str();

        if (NFD_SaveDialogU8_With(&out_path, &args) == NFD_OKAY) {
            requested_filepath_ = out_path;
            NFD_FreePathU8(out_path);
            save_requested_ = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        nfdu8char_t* out_path = nullptr;
        nfdu8filteritem_t filters[] = {{"FURIOUS Project", "furious"}};
        nfdopendialogu8args_t args = {0};
        args.filterList = filters;
        args.filterCount = 1;

        if (NFD_OpenDialogU8_With(&out_path, &args) == NFD_OKAY) {
            requested_filepath_ = out_path;
            NFD_FreePathU8(out_path);
            load_requested_ = true;
        }
    }

    ImGui::End();
}

bool TransportControls::is_playing() const {
    return is_playing_;
}

void TransportControls::set_playing(bool playing) {
    is_playing_ = playing;
}

bool TransportControls::metronome_enabled() const {
    return metronome_enabled_;
}

void TransportControls::set_metronome_enabled(bool enabled) {
    metronome_enabled_ = enabled;
}

bool TransportControls::follow_playhead() const {
    return follow_playhead_;
}

void TransportControls::set_follow_playhead(bool follow) {
    follow_playhead_ = follow;
}

bool TransportControls::loop_enabled() const {
    return loop_enabled_;
}

void TransportControls::set_loop_enabled(bool enabled) {
    loop_enabled_ = enabled;
}

bool TransportControls::reset_requested() {
    bool was_requested = reset_requested_;
    reset_requested_ = false;
    return was_requested;
}

bool TransportControls::save_requested() {
    bool was_requested = save_requested_;
    save_requested_ = false;
    return was_requested;
}

bool TransportControls::load_requested() {
    bool was_requested = load_requested_;
    load_requested_ = false;
    return was_requested;
}

} // namespace furious
