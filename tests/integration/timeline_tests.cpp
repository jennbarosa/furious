#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/ui/timeline.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/project.hpp"

namespace {

struct TimelineTestVars {
    furious::Project project;
    furious::TimelineData timeline_data;
    std::unique_ptr<furious::Timeline> timeline;

    TimelineTestVars() {
        timeline = std::make_unique<furious::Timeline>(project);
        timeline->set_timeline_data(&timeline_data);
    }
};

} // namespace

void RegisterTimelineTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "timeline", "basic_render");
    t->SetVarsDataType<TimelineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test Window")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("Timeline Test Window");
        ctx->WindowFocus("");

        IM_CHECK(true);
    };

    t = IM_REGISTER_TEST(engine, "timeline", "multiple_tracks");
    t->SetVarsDataType<TimelineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test Window")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        vars.timeline_data.add_track("Track 2");
        vars.timeline_data.add_track("Track 3");

        ctx->SetRef("Timeline Test Window");
        ctx->WindowFocus("");

        IM_CHECK_EQ(vars.timeline_data.track_count(), 3u);
    };

    t = IM_REGISTER_TEST(engine, "timeline", "add_clips");
    t->SetVarsDataType<TimelineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test Window")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        furious::TimelineClip clip;
        clip.id = "test-clip-1";
        clip.source_id = "test-source";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 4.0;
        vars.timeline_data.add_clip(clip);

        ctx->SetRef("Timeline Test Window");
        ctx->Yield(); 

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);
        IM_CHECK(vars.timeline_data.find_clip("test-clip-1") != nullptr);
    };

    t = IM_REGISTER_TEST(engine, "timeline", "playhead_position");
    t->SetVarsDataType<TimelineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test Window")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        vars.timeline->set_playhead_position(8.0);

        ctx->SetRef("Timeline Test Window");
        ctx->Yield();

        IM_CHECK_EQ(vars.timeline->playhead_position(), 8.0);
    };

    t = IM_REGISTER_TEST(engine, "timeline", "zoom");
    t->SetVarsDataType<TimelineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test Window")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        float initial_zoom = vars.timeline->zoom();
        vars.timeline->set_zoom(2.0f);

        ctx->SetRef("Timeline Test Window");
        ctx->Yield();

        IM_CHECK_EQ(vars.timeline->zoom(), 2.0f);
        IM_CHECK(vars.timeline->zoom() != initial_zoom);
    };

    t = IM_REGISTER_TEST(engine, "timeline", "clip_selection");
    t->SetVarsDataType<TimelineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test Window")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        furious::TimelineClip clip;
        clip.id = "selectable-clip";
        clip.source_id = "test-source";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 4.0;
        vars.timeline_data.add_clip(clip);

        ctx->SetRef("Timeline Test Window");
        ctx->Yield();

        IM_CHECK(vars.timeline->selected_clip_id().empty());

        vars.timeline->clear_selection();
        IM_CHECK(vars.timeline->selected_clip_id().empty());
    };

    t = IM_REGISTER_TEST(engine, "timeline", "track_operations");
    t->SetVarsDataType<TimelineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test Window")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TimelineTestVars>();

        IM_CHECK_EQ(vars.timeline_data.track_count(), 1u);

        vars.timeline_data.add_track("New Track");
        IM_CHECK_EQ(vars.timeline_data.track_count(), 2u);

        vars.timeline_data.remove_track(1);
        IM_CHECK_EQ(vars.timeline_data.track_count(), 1u);

        ctx->SetRef("Timeline Test Window");
        ctx->Yield();
    };
}
