#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/ui/viewport.hpp"
#include "furious/core/timeline_data.hpp"

namespace {

struct ViewportTestVars {
    furious::Viewport viewport;
    furious::TimelineData timeline_data;

    ViewportTestVars() {
        viewport.set_timeline_data(&timeline_data);
        viewport.set_size(1280, 720);
    }
};

}

void RegisterViewportTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "viewport", "basic_render");
    t->SetVarsDataType<ViewportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Viewport Test")) {
            vars.viewport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("Viewport Test");
        ctx->WindowFocus("");
        IM_CHECK(true);
    };

    t = IM_REGISTER_TEST(engine, "viewport", "resize");
    t->SetVarsDataType<ViewportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Viewport Test")) {
            vars.viewport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();

        ctx->Yield();

        IM_CHECK_GT(vars.viewport.width(), 0.0f);
        IM_CHECK_GT(vars.viewport.height(), 0.0f);
    };

    t = IM_REGISTER_TEST(engine, "viewport", "clip_selection");
    t->SetVarsDataType<ViewportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Viewport Test")) {
            vars.viewport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();

        IM_CHECK(vars.viewport.selected_clip_id().empty());

        vars.viewport.set_selected_clip_id("test-clip");
        ctx->Yield();

        IM_CHECK_EQ(vars.viewport.selected_clip_id(), std::string("test-clip"));
    };

    t = IM_REGISTER_TEST(engine, "viewport", "active_clips_display");
    t->SetVarsDataType<ViewportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Viewport Test")) {
            vars.viewport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();

        furious::TimelineClip clip1;
        clip1.id = "clip-1";
        clip1.source_id = "source-1";
        clip1.track_index = 0;
        clip1.start_beat = 0.0;
        clip1.duration_beats = 4.0;
        vars.timeline_data.add_clip(clip1);

        furious::TimelineClip clip2;
        clip2.id = "clip-2";
        clip2.source_id = "source-2";
        clip2.track_index = 1;
        clip2.start_beat = 0.0;
        clip2.duration_beats = 4.0;
        vars.timeline_data.add_track("Track 2");
        vars.timeline_data.add_clip(clip2);

        auto clips = vars.timeline_data.clips_at_beat(2.0);
        std::vector<const furious::TimelineClip*> const_clips;
        for (auto* c : clips) {
            const_clips.push_back(c);
        }
        vars.viewport.set_active_clips(const_clips);

        ctx->Yield();
        IM_CHECK_EQ(clips.size(), 2u);
    };

    t = IM_REGISTER_TEST(engine, "viewport", "clip_transform_properties");
    t->SetVarsDataType<ViewportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Viewport Test")) {
            vars.viewport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ViewportTestVars>();

        furious::TimelineClip clip;
        clip.id = "transform-clip";
        clip.source_id = "source";
        clip.position_x = 100.0f;
        clip.position_y = 50.0f;
        clip.scale_x = 1.5f;
        clip.scale_y = 1.5f;
        clip.rotation = 45.0f;
        vars.timeline_data.add_clip(clip);

        auto* found = vars.timeline_data.find_clip("transform-clip");
        IM_CHECK(found != nullptr);
        IM_CHECK_EQ(found->position_x, 100.0f);
        IM_CHECK_EQ(found->position_y, 50.0f);
        IM_CHECK_EQ(found->scale_x, 1.5f);
        IM_CHECK_EQ(found->scale_y, 1.5f);
        IM_CHECK_EQ(found->rotation, 45.0f);
    };
}
