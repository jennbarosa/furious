#include "furious/ui/viewport.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>

namespace furious {

namespace {
ImGuiWindowClass& GetViewportWindowClass() {
    static ImGuiWindowClass window_class;
    static bool initialized = false;
    if (!initialized) {
        window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
        initialized = true;
    }
    return window_class;
}
} // namespace

Viewport::Viewport() = default;

void Viewport::render() {
    ImGui::SetNextWindowClass(&GetViewportWindowClass());
    ImGui::Begin("Viewport");

    ImVec2 available = ImGui::GetContentRegionAvail();
    width_ = available.x;
    height_ = available.y;

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(
        canvas_pos,
        ImVec2(canvas_pos.x + width_, canvas_pos.y + height_),
        IM_COL32(30, 30, 30, 255)
    );

    if (video_engine_ && timeline_data_) {
        for (const TimelineClip* clip : active_clips_) {
            if (!clip) continue;

            uint32_t tex_id = video_engine_->get_texture(clip->id);
            if (tex_id == 0) continue;

            int tex_w = video_engine_->get_texture_width(clip->source_id);
            int tex_h = video_engine_->get_texture_height(clip->source_id);

            if (tex_w == 0 || tex_h == 0) continue;

            float scale_x = clip->scale_x;
            float scale_y = clip->scale_y;
            float rotation = clip->rotation;
            float position_x = clip->position_x;
            float position_y = clip->position_y;

            bool flip_h = false;
            bool flip_v = false;

            auto override_it = transform_overrides_.find(clip->id);
            if (override_it != transform_overrides_.end()) {
                const auto& ovr = override_it->second;
                if (ovr.scale_x.has_value()) scale_x = ovr.scale_x.value();
                if (ovr.scale_y.has_value()) scale_y = ovr.scale_y.value();
                if (ovr.rotation.has_value()) rotation = ovr.rotation.value();
                if (ovr.position_x.has_value()) position_x = ovr.position_x.value();
                if (ovr.position_y.has_value()) position_y = ovr.position_y.value();
                if (ovr.flip_h.has_value()) flip_h = ovr.flip_h.value();
                if (ovr.flip_v.has_value()) flip_v = ovr.flip_v.value();
            }

            float scaled_w = static_cast<float>(tex_w) * std::fabs(scale_x);
            float scaled_h = static_cast<float>(tex_h) * std::fabs(scale_y);

            float center_x = canvas_pos.x + position_x + scaled_w * 0.5f;
            float center_y = canvas_pos.y + position_y + scaled_h * 0.5f;

            ImVec2 uv_tl(0, 0);
            ImVec2 uv_tr(1, 0);
            ImVec2 uv_br(1, 1);
            ImVec2 uv_bl(0, 1);

            bool do_flip_h = (scale_x < 0) != flip_h;
            bool do_flip_v = (scale_y < 0) != flip_v;

            if (do_flip_h) {
                std::swap(uv_tl.x, uv_tr.x);
                std::swap(uv_bl.x, uv_br.x);
            }
            if (do_flip_v) {
                std::swap(uv_tl.y, uv_bl.y);
                std::swap(uv_tr.y, uv_br.y);
            }

            if (rotation == 0.0f) {
                float x = canvas_pos.x + position_x;
                float y = canvas_pos.y + position_y;

                draw_list->AddImageQuad(
                    static_cast<ImTextureID>(static_cast<uint64_t>(tex_id)),
                    ImVec2(x, y),
                    ImVec2(x + scaled_w, y),
                    ImVec2(x + scaled_w, y + scaled_h),
                    ImVec2(x, y + scaled_h),
                    uv_tl, uv_tr, uv_br, uv_bl
                );

                if (clip->id == selected_clip_id_) {
                    draw_list->AddRect(
                        ImVec2(x - 2, y - 2),
                        ImVec2(x + scaled_w + 2, y + scaled_h + 2),
                        IM_COL32(100, 180, 255, 255),
                        0.0f, 0, 2.0f
                    );
                }
            } else {
                float rad = rotation * 3.14159265f / 180.0f;
                float cos_r = std::cos(rad);
                float sin_r = std::sin(rad);

                float hw = scaled_w * 0.5f;
                float hh = scaled_h * 0.5f;

                auto rotate_point = [&](float dx, float dy) -> ImVec2 {
                    float rx = dx * cos_r - dy * sin_r;
                    float ry = dx * sin_r + dy * cos_r;
                    return ImVec2(center_x + rx, center_y + ry);
                };

                ImVec2 p1 = rotate_point(-hw, -hh);
                ImVec2 p2 = rotate_point(hw, -hh);
                ImVec2 p3 = rotate_point(hw, hh);
                ImVec2 p4 = rotate_point(-hw, hh);

                draw_list->AddImageQuad(
                    static_cast<ImTextureID>(static_cast<uint64_t>(tex_id)),
                    p1, p2, p3, p4,
                    uv_tl, uv_tr, uv_br, uv_bl
                );

                if (clip->id == selected_clip_id_) {
                    draw_list->AddQuad(p1, p2, p3, p4, IM_COL32(100, 180, 255, 255), 2.0f);
                }
            }
        }
    }

    draw_list->AddRect(
        canvas_pos,
        ImVec2(canvas_pos.x + width_, canvas_pos.y + height_),
        IM_COL32(80, 80, 80, 255)
    );

    ImGui::InvisibleButton("viewport_canvas", ImVec2(width_, height_));

    if (ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_Space)) {
        play_toggle_requested_ = true;
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && timeline_data_) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        dragging_clip_id_.clear();

        for (auto it = active_clips_.rbegin(); it != active_clips_.rend(); ++it) {
            const TimelineClip* clip = *it;
            if (!clip) continue;

            int tex_w = video_engine_ ? video_engine_->get_texture_width(clip->source_id) : 100;
            int tex_h = video_engine_ ? video_engine_->get_texture_height(clip->source_id) : 100;

            float scaled_w = static_cast<float>(tex_w) * clip->scale_x;
            float scaled_h = static_cast<float>(tex_h) * clip->scale_y;

            float x = canvas_pos.x + clip->position_x;
            float y = canvas_pos.y + clip->position_y;

            if (mouse_pos.x >= x && mouse_pos.x <= x + scaled_w &&
                mouse_pos.y >= y && mouse_pos.y <= y + scaled_h) {
                selected_clip_id_ = clip->id;
                dragging_clip_id_ = clip->id;
                dragging_ = true;
                drag_initial_clip_state_ = *clip;
                break;
            }
        }
    }

    if (dragging_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && timeline_data_) {
        if (TimelineClip* clip = timeline_data_->find_clip(dragging_clip_id_)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            clip->position_x += delta.x;
            clip->position_y += delta.y;
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && dragging_) {
        if (timeline_data_) {
            if (TimelineClip* clip = timeline_data_->find_clip(dragging_clip_id_)) {
                if (clip->position_x != drag_initial_clip_state_.position_x ||
                    clip->position_y != drag_initial_clip_state_.position_y) {
                    pending_clip_modification_ = {drag_initial_clip_state_, *clip};
                }
            }
        }
        dragging_ = false;
        dragging_clip_id_.clear();
    }

    ImGui::End();
}

void Viewport::set_size(float width, float height) {
    width_ = width;
    height_ = height;
}

float Viewport::width() const {
    return width_;
}

float Viewport::height() const {
    return height_;
}

bool Viewport::consume_play_toggle_request() {
    bool was_requested = play_toggle_requested_;
    play_toggle_requested_ = false;
    return was_requested;
}

bool Viewport::consume_clip_modification(TimelineClip& old_state, TimelineClip& new_state) {
    if (pending_clip_modification_) {
        old_state = pending_clip_modification_->first;
        new_state = pending_clip_modification_->second;
        pending_clip_modification_.reset();
        return true;
    }
    return false;
}

} // namespace furious
