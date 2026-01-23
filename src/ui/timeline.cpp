#include "furious/ui/timeline.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace furious {

namespace {
ImGuiWindowClass& GetTimelineWindowClass() {
    static ImGuiWindowClass window_class;
    static bool initialized = false;
    if (!initialized) {
        window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
        initialized = true;
    }
    return window_class;
}

double snap_to_grid(double beats, NoteSubdivision subdivision) {
    double grid_size = 1.0 / static_cast<double>(static_cast<int>(subdivision));
    return std::round(beats / grid_size) * grid_size;
}
} // namespace

Timeline::Timeline(Project& project) : project_(project) {}

void Timeline::update(double delta_seconds, bool is_playing) {
    if (is_playing) {
        double delta_beats = project_.tempo().time_to_beats(delta_seconds);
        playhead_beats_ += delta_beats;
    }
}

void Timeline::render() {
    ImGui::SetNextWindowClass(&GetTimelineWindowClass());
    ImGui::Begin("Timeline");

    float time_info_height = ImGui::GetTextLineHeightWithSpacing() + 4.0f;

    ImVec2 available = ImGui::GetContentRegionAvail();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    float canvas_width = available.x - TRACK_HEADER_WIDTH;
    float canvas_height = available.y - time_info_height;
    last_canvas_width_ = canvas_width;
    last_canvas_height_ = canvas_height;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 track_area_pos = ImVec2(canvas_pos.x + TRACK_HEADER_WIDTH, canvas_pos.y);

    draw_list->AddRectFilled(
        track_area_pos,
        ImVec2(track_area_pos.x + canvas_width, track_area_pos.y + canvas_height),
        IM_COL32(40, 40, 45, 255)
    );

    render_track_headers(canvas_pos, canvas_height);
    render_tracks(track_area_pos, canvas_width, canvas_height);
    render_clip_region(track_area_pos, canvas_width, canvas_height);
    render_grid(track_area_pos, canvas_width, canvas_height);
    render_clips(track_area_pos, canvas_width, canvas_height);
    render_playhead(track_area_pos, canvas_width, canvas_height);

    ImGui::InvisibleButton("timeline_canvas", ImVec2(available.x, canvas_height));
    handle_clip_interaction(track_area_pos, canvas_width, canvas_height);
    handle_input(track_area_pos, canvas_width);

    render_time_info();

    if (show_remove_track_popup_) {
        ImGui::OpenPopup("Remove Track?");
        show_remove_track_popup_ = false;
    }

    if (ImGui::BeginPopupModal("Remove Track?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (timeline_data_ && pending_remove_track_index_ < timeline_data_->track_count()) {
            const Track& track = timeline_data_->track(pending_remove_track_index_);
            ImGui::Text("There are clips on the timeline using this track!");
            ImGui::Text("Are you SURE you want to delete \"%s\"?", track.name.c_str());
            ImGui::Spacing();

            if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
                timeline_data_->remove_track(pending_remove_track_index_);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Timeline::render_clip_region(ImVec2 canvas_pos, float canvas_width, float canvas_height) {
    if (clip_duration_beats_ <= 0.0) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float pixels_per_beat = 100.0f * zoom_;
    float clip_end_x = canvas_pos.x + static_cast<float>(clip_duration_beats_) * pixels_per_beat - scroll_offset_;

    float clip_start_x = canvas_pos.x - scroll_offset_;
    if (clip_start_x < canvas_pos.x) clip_start_x = canvas_pos.x;

    if (clip_end_x > canvas_pos.x) {
        float visible_end = std::min(clip_end_x, canvas_pos.x + canvas_width);
        draw_list->AddRectFilled(
            ImVec2(clip_start_x, canvas_pos.y),
            ImVec2(visible_end, canvas_pos.y + canvas_height),
            IM_COL32(50, 50, 60, 255)
        );
    }

    if (clip_end_x >= canvas_pos.x && clip_end_x <= canvas_pos.x + canvas_width) {
        draw_list->AddLine(
            ImVec2(clip_end_x, canvas_pos.y),
            ImVec2(clip_end_x, canvas_pos.y + canvas_height),
            IM_COL32(255, 140, 50, 255),
            4.0f
        );
    }
}

void Timeline::render_grid(ImVec2 canvas_pos, float canvas_width, float canvas_height) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float pixels_per_beat = 100.0f * zoom_;
    int subdivision = static_cast<int>(project_.grid_subdivision());

    float start_beat = scroll_offset_ / pixels_per_beat;
    int start_line = static_cast<int>(std::floor(start_beat * subdivision));
    int end_line = static_cast<int>(std::ceil((scroll_offset_ + canvas_width) / pixels_per_beat * subdivision));

    for (int i = start_line; i <= end_line; ++i) {
        float x = canvas_pos.x + (i / static_cast<float>(subdivision)) * pixels_per_beat - scroll_offset_;

        if (x < canvas_pos.x || x > canvas_pos.x + canvas_width) continue;

        bool is_beat = (i % subdivision) == 0;
        bool is_bar = (i % (subdivision * 4)) == 0;

        ImU32 color;
        float thickness;

        if (is_bar) {
            color = IM_COL32(180, 180, 200, 255);
            thickness = 2.5f;
        } else if (is_beat) {
            color = IM_COL32(90, 90, 100, 255);
            thickness = 1.5f;
        } else {
            color = IM_COL32(55, 55, 60, 255);
            thickness = 1.0f;
        }

        draw_list->AddLine(
            ImVec2(x, canvas_pos.y),
            ImVec2(x, canvas_pos.y + canvas_height),
            color,
            thickness
        );
    }
}

void Timeline::render_playhead(ImVec2 canvas_pos, float canvas_width, float canvas_height) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float pixels_per_beat = 100.0f * zoom_;
    float x = canvas_pos.x + static_cast<float>(playhead_beats_) * pixels_per_beat - scroll_offset_;

    if (x >= canvas_pos.x && x <= canvas_pos.x + canvas_width) {
        draw_list->AddLine(
            ImVec2(x, canvas_pos.y),
            ImVec2(x, canvas_pos.y + canvas_height),
            IM_COL32(255, 100, 100, 255),
            2.0f
        );

        draw_list->AddTriangleFilled(
            ImVec2(x - 6, canvas_pos.y),
            ImVec2(x + 6, canvas_pos.y),
            ImVec2(x, canvas_pos.y + 10),
            IM_COL32(255, 100, 100, 255)
        );
    }
}

void Timeline::set_playhead_position(double beats) {
    playhead_beats_ = beats;
}

double Timeline::playhead_position() const {
    return playhead_beats_;
}

void Timeline::set_zoom(float zoom) {
    zoom_ = std::clamp(zoom, 0.1f, 10.0f);
}

float Timeline::zoom() const {
    return zoom_;
}

void Timeline::set_scroll_offset(float offset) {
    scroll_offset_ = std::max(0.0f, offset);
}

float Timeline::scroll_offset() const {
    return scroll_offset_;
}

void Timeline::set_zoom_y(float zoom) {
    zoom_y_ = std::clamp(zoom, 0.5f, 3.0f);
}

float Timeline::zoom_y() const {
    return zoom_y_;
}

void Timeline::set_scroll_offset_y(float offset) {
    scroll_offset_y_ = std::max(0.0f, offset);
}

float Timeline::scroll_offset_y() const {
    return scroll_offset_y_;
}

void Timeline::handle_input(ImVec2 canvas_pos, float canvas_width) {
    float pixels_per_beat = 100.0f * zoom_;
    is_seeking_ = false;

    if (!dragging_clip_ && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float relative_x = mouse_pos.x - canvas_pos.x + scroll_offset_;
        double clicked_beat = static_cast<double>(relative_x / pixels_per_beat);
        playhead_beats_ = std::max(0.0, clicked_beat);
        is_seeking_ = true;
    }

    if (!dragging_clip_ && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float relative_x = mouse_pos.x - canvas_pos.x + scroll_offset_;
        double clicked_beat = static_cast<double>(relative_x / pixels_per_beat);
        playhead_beats_ = std::max(0.0, clicked_beat);
        is_seeking_ = true;
    }

    if (ImGui::IsItemHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            if (ImGui::GetIO().KeyCtrl) {
                float old_zoom = zoom_;
                set_zoom(zoom_ * (1.0f + wheel * 0.1f));

                ImVec2 mouse_pos = ImGui::GetMousePos();
                float mouse_rel = mouse_pos.x - canvas_pos.x;
                float beat_at_mouse = (scroll_offset_ + mouse_rel) / (100.0f * old_zoom);
                float new_scroll = beat_at_mouse * 100.0f * zoom_ - mouse_rel;
                set_scroll_offset(new_scroll);
            } else if (ImGui::GetIO().KeyAlt) {
                zoom_y_ = std::clamp(zoom_y_ * (1.0f + wheel * 0.1f), 0.5f, 3.0f);
            } else if (ImGui::GetIO().KeyShift) {
                scroll_offset_y_ = std::max(0.0f, scroll_offset_y_ - wheel * 50.0f);
            } else {
                set_scroll_offset(scroll_offset_ - wheel * 50.0f);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            play_toggle_requested_ = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
            if (!selected_clip_id_.empty()) {
                delete_requested_ = true;
                delete_clip_id_ = selected_clip_id_;
            }
        }
    }
}

bool Timeline::consume_play_toggle_request() {
    bool was_requested = play_toggle_requested_;
    play_toggle_requested_ = false;
    return was_requested;
}

bool Timeline::consume_delete_request(std::string& out_clip_id) {
    if (delete_requested_) {
        out_clip_id = delete_clip_id_;
        delete_requested_ = false;
        delete_clip_id_.clear();
        return true;
    }
    return false;
}

bool Timeline::consume_data_modified() {
    bool was_modified = data_modified_;
    data_modified_ = false;
    return was_modified;
}

bool Timeline::consume_clip_modification(TimelineClip& old_state, TimelineClip& new_state) {
    if (!pending_clip_modification_) {
        return false;
    }
    old_state = pending_clip_modification_->first;
    new_state = pending_clip_modification_->second;
    pending_clip_modification_.reset();
    return true;
}

double Timeline::screen_x_to_beats(float screen_x, float canvas_x) const {
    float pixels_per_beat = 100.0f * zoom_;
    float relative_x = screen_x - canvas_x + scroll_offset_;
    return static_cast<double>(relative_x / pixels_per_beat);
}

void Timeline::ensure_playhead_visible() {
    if (!follow_playhead_) return;

    float pixels_per_beat = 100.0f * zoom_;
    float playhead_x = static_cast<float>(playhead_beats_) * pixels_per_beat;
    float visible_start = scroll_offset_;
    float visible_end = scroll_offset_ + last_canvas_width_;

    if (playhead_x < visible_start || playhead_x > visible_end) {
        float new_scroll = playhead_x - last_canvas_width_ * 0.25f;
        set_scroll_offset(new_scroll);
    }
}

void Timeline::render_time_info() {
    double seconds = project_.tempo().beats_to_time(playhead_beats_);
    int minutes = static_cast<int>(seconds) / 60;
    double remaining_seconds = seconds - (minutes * 60);

    int beats_per_measure = 4;
    int total_beats = static_cast<int>(playhead_beats_);
    int measure = (total_beats / beats_per_measure) + 1;
    int beat_in_measure = (total_beats % beats_per_measure) + 1;

    uint64_t frame = static_cast<uint64_t>(seconds * fps_);

    char time_buf[128];
    std::snprintf(time_buf, sizeof(time_buf),
        "Time: %d:%05.2f  |  Measure: %d  Beat: %d  |  Frame: %llu",
        minutes, remaining_seconds, measure, beat_in_measure,
        static_cast<unsigned long long>(frame));

    ImGui::Text("%s", time_buf);
}

void Timeline::render_track_headers(ImVec2 canvas_pos, float canvas_height) {
    if (!timeline_data_) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    size_t track_count = timeline_data_->track_count();
    float track_height = TRACK_HEIGHT * zoom_y_;
    float track_stride = track_height + TRACK_SPACING;

    for (size_t i = 0; i < track_count; ++i) {
        float track_y = canvas_pos.y + static_cast<float>(i) * track_stride - scroll_offset_y_;

        if (track_y + track_height < canvas_pos.y) continue;
        if (track_y > canvas_pos.y + canvas_height) break;

        float visible_top = std::max(track_y, canvas_pos.y);
        float visible_bottom = std::min(track_y + track_height, canvas_pos.y + canvas_height);

        draw_list->AddRectFilled(
            ImVec2(canvas_pos.x, visible_top),
            ImVec2(canvas_pos.x + TRACK_HEADER_WIDTH - 2.0f, visible_bottom),
            IM_COL32(50, 50, 55, 255)
        );

        float text_height = ImGui::GetTextLineHeight();
        float text_y = visible_top + 4.0f;
        if (text_y >= canvas_pos.y &&
            text_y + text_height <= visible_bottom &&
            (visible_bottom - visible_top) >= text_height + 4.0f) {
            const Track& track = timeline_data_->track(i);
            draw_list->AddText(
                ImVec2(canvas_pos.x + 4.0f, text_y),
                IM_COL32(200, 200, 200, 255),
                track.name.c_str()
            );
        }
    }

    float button_y = canvas_pos.y + static_cast<float>(track_count) * track_stride - scroll_offset_y_;
    if (button_y >= canvas_pos.y && button_y < canvas_pos.y + canvas_height - 20.0f) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 btn_size(20.0f, 16.0f);

        ImVec2 add_btn_pos(canvas_pos.x + 4.0f, button_y + 4.0f);
        ImVec2 add_btn_max(add_btn_pos.x + btn_size.x, add_btn_pos.y + btn_size.y);
        bool add_hovered = mouse_pos.x >= add_btn_pos.x && mouse_pos.x <= add_btn_max.x &&
                           mouse_pos.y >= add_btn_pos.y && mouse_pos.y <= add_btn_max.y;

        ImU32 add_btn_color = add_hovered ? IM_COL32(80, 80, 90, 255) : IM_COL32(60, 60, 70, 255);
        draw_list->AddRectFilled(add_btn_pos, add_btn_max, add_btn_color, 3.0f);
        draw_list->AddText(ImVec2(add_btn_pos.x + 6.0f, add_btn_pos.y + 1.0f), IM_COL32(200, 200, 200, 255), "+");

        if (add_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            timeline_data_->add_track();
        }

        if (track_count > 1) {
            ImVec2 remove_btn_pos(add_btn_pos.x + btn_size.x + 4.0f, button_y + 4.0f);
            ImVec2 remove_btn_max(remove_btn_pos.x + btn_size.x, remove_btn_pos.y + btn_size.y);
            bool remove_hovered = mouse_pos.x >= remove_btn_pos.x && mouse_pos.x <= remove_btn_max.x &&
                                  mouse_pos.y >= remove_btn_pos.y && mouse_pos.y <= remove_btn_max.y;

            ImU32 remove_btn_color = remove_hovered ? IM_COL32(100, 60, 60, 255) : IM_COL32(70, 50, 50, 255);
            draw_list->AddRectFilled(remove_btn_pos, remove_btn_max, remove_btn_color, 3.0f);
            draw_list->AddText(ImVec2(remove_btn_pos.x + 7.0f, remove_btn_pos.y + 1.0f), IM_COL32(200, 200, 200, 255), "-");

            if (remove_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                size_t last_track = track_count - 1;
                auto clips_on_track = timeline_data_->clips_on_track(last_track);
                if (clips_on_track.empty()) {
                    timeline_data_->remove_track(last_track);
                } else {
                    pending_remove_track_index_ = last_track;
                    show_remove_track_popup_ = true;
                }
            }
        }
    }
}

void Timeline::render_tracks(ImVec2 canvas_pos, float canvas_width, float canvas_height) {
    if (!timeline_data_) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    size_t track_count = timeline_data_->track_count();
    float track_height = TRACK_HEIGHT * zoom_y_;
    float track_stride = track_height + TRACK_SPACING;

    for (size_t i = 0; i < track_count; ++i) {
        float track_y = canvas_pos.y + static_cast<float>(i) * track_stride - scroll_offset_y_;

        if (track_y + track_height < canvas_pos.y) continue;
        if (track_y > canvas_pos.y + canvas_height) break;

        float visible_top = std::max(track_y, canvas_pos.y);
        float visible_bottom = std::min(track_y + track_height, canvas_pos.y + canvas_height);

        draw_list->AddRectFilled(
            ImVec2(canvas_pos.x, visible_top),
            ImVec2(canvas_pos.x + canvas_width, visible_bottom),
            IM_COL32(35, 35, 40, 255)
        );

        if (track_y + track_height >= canvas_pos.y && track_y + track_height <= canvas_pos.y + canvas_height) {
            draw_list->AddLine(
                ImVec2(canvas_pos.x, track_y + track_height),
                ImVec2(canvas_pos.x + canvas_width, track_y + track_height),
                IM_COL32(60, 60, 65, 255),
                1.0f
            );
        }
    }
}

void Timeline::render_clips(ImVec2 canvas_pos, float canvas_width, float canvas_height) {
    if (!timeline_data_) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float pixels_per_beat = 100.0f * zoom_;
    float track_height = TRACK_HEIGHT * zoom_y_;
    float track_stride = track_height + TRACK_SPACING;

    for (const auto& clip : timeline_data_->clips()) {
        float track_y = canvas_pos.y + static_cast<float>(clip.track_index) * track_stride - scroll_offset_y_;

        if (track_y + track_height < canvas_pos.y) continue;
        if (track_y > canvas_pos.y + canvas_height) continue;

        float clip_x = canvas_pos.x + static_cast<float>(clip.start_beat) * pixels_per_beat - scroll_offset_;
        float clip_w = static_cast<float>(clip.duration_beats) * pixels_per_beat;
        float clip_end_x = clip_x + clip_w;

        if (clip_end_x < canvas_pos.x || clip_x > canvas_pos.x + canvas_width) continue;

        clip_x = std::max(clip_x, canvas_pos.x);
        clip_end_x = std::min(clip_end_x, canvas_pos.x + canvas_width);

        float visible_top = std::max(track_y + 2.0f, canvas_pos.y);
        float visible_bottom = std::min(track_y + track_height - 2.0f, canvas_pos.y + canvas_height);

        bool is_selected = (clip.id == selected_clip_id_);

        ImU32 clip_color = is_selected
            ? IM_COL32(120, 180, 240, 255)
            : IM_COL32(80, 140, 200, 255);

        draw_list->AddRectFilled(
            ImVec2(clip_x, visible_top),
            ImVec2(clip_end_x, visible_bottom),
            clip_color
        );

        draw_list->AddRect(
            ImVec2(clip_x, visible_top),
            ImVec2(clip_end_x, visible_bottom),
            IM_COL32(255, 255, 255, is_selected ? 200 : 100),
            0.0f, 0, 1.0f
        );

        std::string clip_name = "Clip";
        if (source_library_) {
            if (const MediaSource* src = source_library_->find_source(clip.source_id)) {
                clip_name = src->name;
            }
        }

        float text_x = std::max(clip_x + 4.0f, canvas_pos.x + 4.0f);
        float text_y = visible_top + 4.0f;
        float text_height = ImGui::GetTextLineHeight();
        if (text_x < clip_end_x - 20.0f &&
            text_y >= canvas_pos.y &&
            text_y + text_height <= visible_bottom &&
            (visible_bottom - visible_top) >= text_height + 4.0f) {
            draw_list->AddText(
                ImVec2(text_x, text_y),
                IM_COL32(255, 255, 255, 220),
                clip_name.c_str()
            );
        }
    }
}

void Timeline::handle_clip_interaction(ImVec2 canvas_pos, float canvas_width, float canvas_height) {
    if (!timeline_data_) return;

    float pixels_per_beat = 100.0f * zoom_;
    float track_height = TRACK_HEIGHT * zoom_y_;
    float track_stride = track_height + TRACK_SPACING;
    ImVec2 mouse_pos = ImGui::GetMousePos();

    constexpr double MIN_CLIP_DURATION = 0.25;

    // cursor feedback for edge hover (only when not dragging)
    if (!dragging_clip_) {
        bool on_edge = false;
        for (const auto& clip : timeline_data_->clips()) {
            float track_y = canvas_pos.y + static_cast<float>(clip.track_index) * track_stride - scroll_offset_y_;
            float clip_x = canvas_pos.x + static_cast<float>(clip.start_beat) * pixels_per_beat - scroll_offset_;
            float clip_w = static_cast<float>(clip.duration_beats) * pixels_per_beat;

            if (mouse_pos.y >= track_y && mouse_pos.y <= track_y + track_height) {
                // check left edge
                if (mouse_pos.x >= clip_x - EDGE_HIT_ZONE / 2 && mouse_pos.x <= clip_x + EDGE_HIT_ZONE / 2) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                    on_edge = true;
                    break;
                }
                // check right edge
                float right_edge = clip_x + clip_w;
                if (mouse_pos.x >= right_edge - EDGE_HIT_ZONE / 2 && mouse_pos.x <= right_edge + EDGE_HIT_ZONE / 2) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                    on_edge = true;
                    break;
                }
            }
        }
    }

    // handle click to start drag
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().KeyCtrl) {
        selected_clip_id_.clear();
        drag_mode_ = DragMode::None;

        for (auto& clip : timeline_data_->clips()) {
            float track_y = canvas_pos.y + static_cast<float>(clip.track_index) * track_stride - scroll_offset_y_;
            float clip_x = canvas_pos.x + static_cast<float>(clip.start_beat) * pixels_per_beat - scroll_offset_;
            float clip_w = static_cast<float>(clip.duration_beats) * pixels_per_beat;

            if (mouse_pos.y >= track_y && mouse_pos.y <= track_y + track_height) {
                float right_edge = clip_x + clip_w;
                float dist_to_left = std::abs(mouse_pos.x - clip_x);
                float dist_to_right = std::abs(mouse_pos.x - right_edge);

                bool on_left_edge = mouse_pos.x >= clip_x - EDGE_HIT_ZONE / 2 && mouse_pos.x <= clip_x + EDGE_HIT_ZONE / 2;
                bool on_right_edge = mouse_pos.x >= right_edge - EDGE_HIT_ZONE / 2 && mouse_pos.x <= right_edge + EDGE_HIT_ZONE / 2;
                bool in_clip = mouse_pos.x >= clip_x && mouse_pos.x <= clip_x + clip_w;

                if (on_left_edge && on_right_edge) {
                    if (dist_to_right < dist_to_left) {
                        on_left_edge = false;
                    } else {
                        on_right_edge = false;
                    }
                }

                if (on_left_edge) {
                    selected_clip_id_ = clip.id;
                    dragging_clip_id_ = clip.id;
                    dragging_clip_ = true;
                    drag_initial_clip_state_ = clip;
                    drag_mode_ = DragMode::TrimLeft;
                    drag_initial_start_beat_ = clip.start_beat;
                    drag_initial_duration_ = clip.duration_beats;
                    drag_initial_source_start_ = clip.source_start_seconds;
                    drag_mouse_start_x_ = mouse_pos.x;
                    break;
                }

                if (on_right_edge) {
                    selected_clip_id_ = clip.id;
                    dragging_clip_id_ = clip.id;
                    dragging_clip_ = true;
                    drag_initial_clip_state_ = clip;
                    drag_mode_ = DragMode::TrimRight;
                    drag_initial_start_beat_ = clip.start_beat;
                    drag_initial_duration_ = clip.duration_beats;
                    drag_mouse_start_x_ = mouse_pos.x;
                    break;
                }

                if (in_clip) {
                    selected_clip_id_ = clip.id;
                    dragging_clip_id_ = clip.id;
                    dragging_clip_ = true;
                    drag_initial_clip_state_ = clip;
                    drag_mode_ = DragMode::Move;
                    drag_initial_start_beat_ = clip.start_beat;
                    drag_mouse_start_x_ = mouse_pos.x;
                    break;
                }
            }
        }
    }

    if (dragging_clip_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        if (TimelineClip* clip = timeline_data_->find_clip(dragging_clip_id_)) {
            float delta_x = mouse_pos.x - drag_mouse_start_x_;
            double delta_beats = static_cast<double>(delta_x) / pixels_per_beat;

            bool snap = project_.snap_enabled();
            NoteSubdivision grid = project_.grid_subdivision();

            switch (drag_mode_) {
                case DragMode::Move: {
                    double new_start = drag_initial_start_beat_ + delta_beats;

                    if (snap) {
                        new_start = snap_to_grid(new_start, grid);
                    }
                    clip->start_beat = std::max(0.0, new_start);

                    float mouse_y_rel = mouse_pos.y - canvas_pos.y + scroll_offset_y_;
                    size_t new_track = static_cast<size_t>(mouse_y_rel / track_stride);
                    new_track = std::min(new_track, timeline_data_->track_count() - 1);
                    clip->track_index = new_track;
                    break;
                }

                case DragMode::TrimLeft: {
                    double end_beat = drag_initial_start_beat_ + drag_initial_duration_;
                    double new_start_beat = drag_initial_start_beat_ + delta_beats;

                    if (snap) {
                        new_start_beat = snap_to_grid(new_start_beat, grid);
                    }

                    new_start_beat = std::max(0.0, new_start_beat);
                    if (end_beat - new_start_beat < MIN_CLIP_DURATION) {
                        new_start_beat = end_beat - MIN_CLIP_DURATION;
                    }

                    double start_beat_change = new_start_beat - drag_initial_start_beat_;
                    double source_start_change_seconds = project_.tempo().beats_to_time(start_beat_change);
                    double new_source_start = drag_initial_source_start_ + source_start_change_seconds;

                    if (new_source_start < 0.0) {
                        new_source_start = 0.0;
                        double max_change_seconds = -drag_initial_source_start_;
                        double max_change_beats = project_.tempo().time_to_beats(max_change_seconds);
                        new_start_beat = drag_initial_start_beat_ + max_change_beats;
                    }

                    clip->start_beat = new_start_beat;
                    clip->duration_beats = end_beat - new_start_beat;
                    clip->source_start_seconds = new_source_start;
                    break;
                }

                case DragMode::TrimRight: {
                    double new_end_beat = drag_initial_start_beat_ + drag_initial_duration_ + delta_beats;

                    if (snap) {
                        new_end_beat = snap_to_grid(new_end_beat, grid);
                    }

                    double new_duration = new_end_beat - drag_initial_start_beat_;
                    new_duration = std::max(MIN_CLIP_DURATION, new_duration);

                    clip->duration_beats = new_duration;
                    break;
                }

                case DragMode::None:
                    break;
            }
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (dragging_clip_) {
            if (const TimelineClip* clip = timeline_data_->find_clip(dragging_clip_id_)) {
                bool changed = clip->start_beat != drag_initial_clip_state_.start_beat ||
                               clip->duration_beats != drag_initial_clip_state_.duration_beats ||
                               clip->track_index != drag_initial_clip_state_.track_index ||
                               clip->source_start_seconds != drag_initial_clip_state_.source_start_seconds;
                if (changed) {
                    pending_clip_modification_ = std::make_pair(drag_initial_clip_state_, *clip);
                }
            }
            data_modified_ = true;
        }
        dragging_clip_ = false;
        dragging_clip_id_.clear();
        drag_mode_ = DragMode::None;
    }
}

} // namespace furious
