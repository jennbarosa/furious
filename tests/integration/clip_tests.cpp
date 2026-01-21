#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/ui/timeline.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/timeline_clip.hpp"
#include "furious/core/project.hpp"

namespace {

struct ClipTestVars {
    furious::Project project;
    furious::TimelineData timeline_data;
    std::unique_ptr<furious::Timeline> timeline;

    ClipTestVars() {
        timeline = std::make_unique<furious::Timeline>(project);
        timeline->set_timeline_data(&timeline_data);
    }
};

furious::TimelineClip make_clip(const std::string& id, size_t track, double start, double duration) {
    furious::TimelineClip clip;
    clip.id = id;
    clip.source_id = "test-source";
    clip.track_index = track;
    clip.start_beat = start;
    clip.duration_beats = duration;
    return clip;
}

}

void RegisterClipTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "clips", "add_remove");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Clip Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 0u);

        vars.timeline_data.add_clip(make_clip("clip-1", 0, 0.0, 4.0));
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);

        vars.timeline_data.add_clip(make_clip("clip-2", 0, 4.0, 4.0));
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 2u);

        vars.timeline_data.remove_clip("clip-1");
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);
        IM_CHECK(vars.timeline_data.find_clip("clip-1") == nullptr);
        IM_CHECK(vars.timeline_data.find_clip("clip-2") != nullptr);
    };

    t = IM_REGISTER_TEST(engine, "clips", "move_position");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Clip Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        vars.timeline_data.add_clip(make_clip("movable", 0, 0.0, 4.0));
        auto* clip = vars.timeline_data.find_clip("movable");
        IM_CHECK(clip != nullptr);

        clip->start_beat = 8.0;
        ctx->Yield();

        IM_CHECK_EQ(clip->start_beat, 8.0);
        IM_CHECK_EQ(clip->end_beat(), 12.0);
    };

    t = IM_REGISTER_TEST(engine, "clips", "change_track");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Clip Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        vars.timeline_data.add_track("Track 2");
        vars.timeline_data.add_track("Track 3");
        vars.timeline_data.add_clip(make_clip("track-mover", 0, 0.0, 4.0));

        auto* clip = vars.timeline_data.find_clip("track-mover");
        IM_CHECK_EQ(clip->track_index, 0u);

        clip->track_index = 1;
        ctx->Yield();
        IM_CHECK_EQ(clip->track_index, 1u);

        clip->track_index = 2;
        ctx->Yield();
        IM_CHECK_EQ(clip->track_index, 2u);
    };

    t = IM_REGISTER_TEST(engine, "clips", "trim_right_edge");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Clip Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        vars.timeline_data.add_clip(make_clip("trimmable", 0, 0.0, 8.0));
        auto* clip = vars.timeline_data.find_clip("trimmable");

        IM_CHECK_EQ(clip->duration_beats, 8.0);
        IM_CHECK_EQ(clip->start_beat, 0.0);

        clip->duration_beats = 4.0;
        ctx->Yield();

        IM_CHECK_EQ(clip->duration_beats, 4.0);
        IM_CHECK_EQ(clip->start_beat, 0.0);
        IM_CHECK_EQ(clip->end_beat(), 4.0);
    };

    t = IM_REGISTER_TEST(engine, "clips", "trim_left_edge");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Clip Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        auto clip = make_clip("left-trim", 0, 0.0, 8.0);
        clip.source_start_seconds = 0.0;
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("left-trim");
        double original_end = c->end_beat();

        c->start_beat = 2.0;
        c->duration_beats = 6.0;
        c->source_start_seconds = 1.0;
        ctx->Yield();

        IM_CHECK_EQ(c->start_beat, 2.0);
        IM_CHECK_EQ(c->duration_beats, 6.0);
        IM_CHECK_EQ(c->end_beat(), original_end);
        IM_CHECK_EQ(c->source_start_seconds, 1.0);
    };

    t = IM_REGISTER_TEST(engine, "clips", "contains_beat");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Clip Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        vars.timeline_data.add_clip(make_clip("range-test", 0, 4.0, 4.0));
        auto* clip = vars.timeline_data.find_clip("range-test");

        IM_CHECK(clip->contains_beat(4.0));
        IM_CHECK(clip->contains_beat(6.0));
        IM_CHECK(clip->contains_beat(7.99));
        IM_CHECK(!clip->contains_beat(3.99));
        IM_CHECK(!clip->contains_beat(8.0));
        IM_CHECK(!clip->contains_beat(10.0));
    };

    t = IM_REGISTER_TEST(engine, "clips", "clips_at_beat");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Clip Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        vars.timeline_data.add_track("Track 2");
        vars.timeline_data.add_clip(make_clip("a", 0, 0.0, 4.0));
        vars.timeline_data.add_clip(make_clip("b", 1, 2.0, 4.0));
        vars.timeline_data.add_clip(make_clip("c", 0, 8.0, 4.0));

        auto at_0 = vars.timeline_data.clips_at_beat(0.0);
        IM_CHECK_EQ(at_0.size(), 1u);

        auto at_3 = vars.timeline_data.clips_at_beat(3.0);
        IM_CHECK_EQ(at_3.size(), 2u);

        auto at_6 = vars.timeline_data.clips_at_beat(6.0);
        IM_CHECK_EQ(at_6.size(), 0u);

        auto at_10 = vars.timeline_data.clips_at_beat(10.0);
        IM_CHECK_EQ(at_10.size(), 1u);
    };

    t = IM_REGISTER_TEST(engine, "clips", "clips_on_track");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Clip Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        vars.timeline_data.add_track("Track 2");
        vars.timeline_data.add_clip(make_clip("t0-a", 0, 0.0, 4.0));
        vars.timeline_data.add_clip(make_clip("t0-b", 0, 4.0, 4.0));
        vars.timeline_data.add_clip(make_clip("t1-a", 1, 0.0, 8.0));

        auto track0 = vars.timeline_data.clips_on_track(0);
        IM_CHECK_EQ(track0.size(), 2u);

        auto track1 = vars.timeline_data.clips_on_track(1);
        IM_CHECK_EQ(track1.size(), 1u);

        auto track2 = vars.timeline_data.clips_on_track(2);
        IM_CHECK_EQ(track2.size(), 0u);
    };

    t = IM_REGISTER_TEST(engine, "clips", "transform_properties");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Clip Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        furious::TimelineClip clip;
        clip.id = "transform";
        clip.source_id = "src";
        clip.position_x = 0.0f;
        clip.position_y = 0.0f;
        clip.scale_x = 1.0f;
        clip.scale_y = 1.0f;
        clip.rotation = 0.0f;
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("transform");

        c->position_x = 100.0f;
        c->position_y = -50.0f;
        IM_CHECK_EQ(c->position_x, 100.0f);
        IM_CHECK_EQ(c->position_y, -50.0f);

        c->scale_x = 2.0f;
        c->scale_y = 0.5f;
        IM_CHECK_EQ(c->scale_x, 2.0f);
        IM_CHECK_EQ(c->scale_y, 0.5f);

        c->rotation = 90.0f;
        IM_CHECK_EQ(c->rotation, 90.0f);

        c->rotation = -45.0f;
        IM_CHECK_EQ(c->rotation, -45.0f);
    };

    t = IM_REGISTER_TEST(engine, "clips", "effects_system");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Clip Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        furious::TimelineClip clip;
        clip.id = "effects-test";
        clip.source_id = "src";
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("effects-test");

        IM_CHECK_EQ(c->effects.empty(), true);

        furious::ClipEffect effect;
        effect.effect_id = "auto_ytpmv";
        effect.parameters["sync_period"] = "quarter";
        effect.enabled = true;
        c->effects.push_back(effect);

        IM_CHECK_EQ(c->effects.size(), 1u);
        IM_CHECK_EQ(c->effects[0].effect_id, "auto_ytpmv");
        IM_CHECK_EQ(c->effects[0].parameters["sync_period"], "quarter");

        c->effects[0].parameters["sync_period"] = "eighth";
        IM_CHECK_EQ(c->effects[0].parameters["sync_period"], "eighth");

        c->effects[0].parameters["sync_period"] = "sixteenth";
        IM_CHECK_EQ(c->effects[0].parameters["sync_period"], "sixteenth");

        c->effects[0].parameters["sync_period"] = "half";
        IM_CHECK_EQ(c->effects[0].parameters["sync_period"], "half");

        c->effects[0].parameters["sync_period"] = "measure";
        IM_CHECK_EQ(c->effects[0].parameters["sync_period"], "measure");
    };

    t = IM_REGISTER_TEST(engine, "clips", "delete_via_timeline");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Clip Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        vars.timeline_data.add_clip(make_clip("to-delete", 0, 0.0, 4.0));
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);

        std::string deleted_id;
        bool has_delete = vars.timeline->consume_delete_request(deleted_id);
        IM_CHECK_EQ(has_delete, false);

        vars.timeline_data.remove_clip("to-delete");
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 0u);
    };

    t = IM_REGISTER_TEST(engine, "clips", "multiple_clips_same_track");
    t->SetVarsDataType<ClipTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Clip Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<ClipTestVars>();

        for (int i = 0; i < 8; ++i) {
            vars.timeline_data.add_clip(make_clip(
                "clip-" + std::to_string(i),
                0,
                static_cast<double>(i * 4),
                4.0
            ));
        }

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 8u);

        auto track_clips = vars.timeline_data.clips_on_track(0);
        IM_CHECK_EQ(track_clips.size(), 8u);

        for (size_t i = 0; i < track_clips.size() - 1; ++i) {
            IM_CHECK_LE(track_clips[i]->start_beat, track_clips[i + 1]->start_beat);
        }
    };
}
