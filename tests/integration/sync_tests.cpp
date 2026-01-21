#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/ui/timeline.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/project.hpp"
#include "furious/core/tempo.hpp"
#include <cmath>

namespace {

struct SyncTestVars {
    furious::Project project;
    furious::TimelineData timeline_data;
    std::unique_ptr<furious::Timeline> timeline;

    SyncTestVars() {
        timeline = std::make_unique<furious::Timeline>(project);
        timeline->set_timeline_data(&timeline_data);
    }
};

}

void RegisterSyncTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "sync", "bpm_time_conversion_60");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(60.0);

        IM_CHECK_EQ(vars.project.tempo().beat_duration_seconds(), 1.0);
        IM_CHECK_EQ(vars.project.tempo().time_to_beats(1.0), 1.0);
        IM_CHECK_EQ(vars.project.tempo().time_to_beats(4.0), 4.0);
        IM_CHECK_EQ(vars.project.tempo().beats_to_time(1.0), 1.0);
        IM_CHECK_EQ(vars.project.tempo().beats_to_time(4.0), 4.0);
    };

    t = IM_REGISTER_TEST(engine, "sync", "bpm_time_conversion_120");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);

        IM_CHECK_EQ(vars.project.tempo().beat_duration_seconds(), 0.5);
        IM_CHECK_EQ(vars.project.tempo().time_to_beats(1.0), 2.0);
        IM_CHECK_EQ(vars.project.tempo().time_to_beats(0.5), 1.0);
        IM_CHECK_EQ(vars.project.tempo().beats_to_time(2.0), 1.0);
        IM_CHECK_EQ(vars.project.tempo().beats_to_time(4.0), 2.0);
    };

    t = IM_REGISTER_TEST(engine, "sync", "bpm_time_conversion_140");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(140.0);

        double expected_beat_duration = 60.0 / 140.0;
        double actual = vars.project.tempo().beat_duration_seconds();
        IM_CHECK(std::fabs(actual - expected_beat_duration) < 0.0001);

        double time_for_4_beats = 4.0 * expected_beat_duration;
        double beats_time = vars.project.tempo().beats_to_time(4.0);
        IM_CHECK(std::fabs(beats_time - time_for_4_beats) < 0.0001);
        double time_beats = vars.project.tempo().time_to_beats(time_for_4_beats);
        IM_CHECK(std::fabs(time_beats - 4.0) < 0.0001);
    };

    t = IM_REGISTER_TEST(engine, "sync", "grid_quarter_note");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);
        vars.project.set_grid_subdivision(furious::NoteSubdivision::Quarter);

        double beat_duration = vars.project.tempo().beat_duration_seconds();
        double subdiv_duration = vars.project.tempo().subdivision_duration_seconds(furious::NoteSubdivision::Quarter);

        IM_CHECK_EQ(subdiv_duration, beat_duration);
        IM_CHECK_EQ(subdiv_duration, 0.5);
    };

    t = IM_REGISTER_TEST(engine, "sync", "grid_eighth_note");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);
        vars.project.set_grid_subdivision(furious::NoteSubdivision::Eighth);

        double beat_duration = vars.project.tempo().beat_duration_seconds();
        double subdiv_duration = vars.project.tempo().subdivision_duration_seconds(furious::NoteSubdivision::Eighth);

        IM_CHECK_EQ(subdiv_duration, beat_duration / 2.0);
        IM_CHECK_EQ(subdiv_duration, 0.25);
    };

    t = IM_REGISTER_TEST(engine, "sync", "grid_sixteenth_note");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);
        vars.project.set_grid_subdivision(furious::NoteSubdivision::Sixteenth);

        double beat_duration = vars.project.tempo().beat_duration_seconds();
        double subdiv_duration = vars.project.tempo().subdivision_duration_seconds(furious::NoteSubdivision::Sixteenth);

        IM_CHECK_EQ(subdiv_duration, beat_duration / 4.0);
        IM_CHECK_EQ(subdiv_duration, 0.125);
    };

    t = IM_REGISTER_TEST(engine, "sync", "playhead_beat_sync");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sync Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);
        vars.timeline->set_playhead_position(0.0);
        ctx->Yield();

        IM_CHECK_EQ(vars.timeline->playhead_position(), 0.0);

        vars.timeline->set_playhead_position(4.0);
        ctx->Yield();
        IM_CHECK_EQ(vars.timeline->playhead_position(), 4.0);

        vars.timeline->set_playhead_position(8.5);
        ctx->Yield();
        IM_CHECK_EQ(vars.timeline->playhead_position(), 8.5);
    };

    t = IM_REGISTER_TEST(engine, "sync", "playhead_update_while_playing");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sync Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);
        vars.timeline->set_playhead_position(0.0);

        double initial = vars.timeline->playhead_position();
        vars.timeline->update(0.5, true);
        double after_update = vars.timeline->playhead_position();

        IM_CHECK_GT(after_update, initial);
        IM_CHECK_EQ(after_update, 1.0);
    };

    t = IM_REGISTER_TEST(engine, "sync", "playhead_stationary_when_stopped");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sync Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);
        vars.timeline->set_playhead_position(4.0);

        double initial = vars.timeline->playhead_position();
        vars.timeline->update(1.0, false);
        double after_update = vars.timeline->playhead_position();

        IM_CHECK_EQ(after_update, initial);
    };

    t = IM_REGISTER_TEST(engine, "sync", "clip_sync_with_bpm");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);

        furious::TimelineClip clip;
        clip.id = "sync-clip";
        clip.source_id = "src";
        clip.start_beat = 4.0;
        clip.duration_beats = 4.0;
        vars.timeline_data.add_clip(clip);

        double start_time = vars.project.tempo().beats_to_time(4.0);
        double end_time = vars.project.tempo().beats_to_time(8.0);

        IM_CHECK_EQ(start_time, 2.0);
        IM_CHECK_EQ(end_time, 4.0);

        vars.project.tempo().set_bpm(60.0);
        start_time = vars.project.tempo().beats_to_time(4.0);
        end_time = vars.project.tempo().beats_to_time(8.0);

        IM_CHECK_EQ(start_time, 4.0);
        IM_CHECK_EQ(end_time, 8.0);
    };

    t = IM_REGISTER_TEST(engine, "sync", "effect_sync_periods");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.tempo().set_bpm(120.0);

        furious::TimelineClip clip;
        clip.id = "effect-sync";
        clip.source_id = "src";
        clip.duration_beats = 8.0;

        furious::ClipEffect effect;
        effect.effect_id = "auto_ytpmv";
        effect.parameters["sync_period"] = "quarter";
        effect.enabled = true;
        clip.effects.push_back(effect);

        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("effect-sync");

        c->effects[0].parameters["sync_period"] = "quarter";
        double period_seconds = vars.project.tempo().beats_to_time(1.0);
        IM_CHECK_EQ(period_seconds, 0.5);

        c->effects[0].parameters["sync_period"] = "eighth";
        period_seconds = vars.project.tempo().beats_to_time(0.5);
        IM_CHECK_EQ(period_seconds, 0.25);

        c->effects[0].parameters["sync_period"] = "sixteenth";
        period_seconds = vars.project.tempo().beats_to_time(0.25);
        IM_CHECK_EQ(period_seconds, 0.125);
    };

    t = IM_REGISTER_TEST(engine, "sync", "measure_calculation");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        double beat = 0.0;
        int measure = static_cast<int>(beat / 4.0) + 1;
        int beat_in_measure = static_cast<int>(std::fmod(beat, 4.0)) + 1;
        IM_CHECK_EQ(measure, 1);
        IM_CHECK_EQ(beat_in_measure, 1);

        beat = 4.0;
        measure = static_cast<int>(beat / 4.0) + 1;
        beat_in_measure = static_cast<int>(std::fmod(beat, 4.0)) + 1;
        IM_CHECK_EQ(measure, 2);
        IM_CHECK_EQ(beat_in_measure, 1);

        beat = 7.0;
        measure = static_cast<int>(beat / 4.0) + 1;
        beat_in_measure = static_cast<int>(std::fmod(beat, 4.0)) + 1;
        IM_CHECK_EQ(measure, 2);
        IM_CHECK_EQ(beat_in_measure, 4);

        beat = 16.0;
        measure = static_cast<int>(beat / 4.0) + 1;
        beat_in_measure = static_cast<int>(std::fmod(beat, 4.0)) + 1;
        IM_CHECK_EQ(measure, 5);
        IM_CHECK_EQ(beat_in_measure, 1);
    };

    t = IM_REGISTER_TEST(engine, "sync", "frame_calculation");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Sync Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.project.set_fps(30.0);
        vars.project.tempo().set_bpm(120.0);

        double time_at_beat_4 = vars.project.tempo().beats_to_time(4.0);
        int frame = static_cast<int>(time_at_beat_4 * vars.project.fps());
        IM_CHECK_EQ(frame, 60);

        vars.project.set_fps(60.0);
        frame = static_cast<int>(time_at_beat_4 * vars.project.fps());
        IM_CHECK_EQ(frame, 120);

        vars.project.set_fps(24.0);
        frame = static_cast<int>(time_at_beat_4 * vars.project.fps());
        IM_CHECK_EQ(frame, 48);
    };

    t = IM_REGISTER_TEST(engine, "sync", "timeline_zoom_levels");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sync Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.timeline->set_zoom(1.0f);
        ctx->Yield();
        IM_CHECK_EQ(vars.timeline->zoom(), 1.0f);

        vars.timeline->set_zoom(0.1f);
        ctx->Yield();
        IM_CHECK_GE(vars.timeline->zoom(), 0.1f);

        vars.timeline->set_zoom(10.0f);
        ctx->Yield();
        IM_CHECK_LE(vars.timeline->zoom(), 10.0f);
    };

    t = IM_REGISTER_TEST(engine, "sync", "timeline_scroll");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sync Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.timeline->set_scroll_offset(0.0f);
        ctx->Yield();
        IM_CHECK_EQ(vars.timeline->scroll_offset(), 0.0f);

        vars.timeline->set_scroll_offset(100.0f);
        ctx->Yield();
        IM_CHECK_EQ(vars.timeline->scroll_offset(), 100.0f);

        vars.timeline->set_scroll_offset_y(50.0f);
        ctx->Yield();
        IM_CHECK_EQ(vars.timeline->scroll_offset_y(), 50.0f);
    };

    t = IM_REGISTER_TEST(engine, "sync", "follow_playhead");
    t->SetVarsDataType<SyncTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sync Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SyncTestVars>();

        vars.timeline->set_follow_playhead(true);
        IM_CHECK_EQ(vars.timeline->follow_playhead(), true);

        vars.timeline->set_follow_playhead(false);
        IM_CHECK_EQ(vars.timeline->follow_playhead(), false);
    };
}
