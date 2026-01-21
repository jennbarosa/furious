#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/ui/transport_controls.hpp"
#include "furious/core/project.hpp"
#include "furious/core/tempo.hpp"
#include <cmath>

namespace {

struct TransportTestVars {
    furious::Project project;
    furious::TransportControls transport;

    TransportTestVars() : transport(project) {}
};

}

void RegisterTransportTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "transport", "basic_render");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("Transport Test");
        ctx->WindowFocus("");
        IM_CHECK(true);
    };

    t = IM_REGISTER_TEST(engine, "transport", "play_stop_toggle");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        IM_CHECK_EQ(vars.transport.is_playing(), false);

        vars.transport.set_playing(true);
        ctx->Yield();
        IM_CHECK_EQ(vars.transport.is_playing(), true);

        vars.transport.set_playing(false);
        ctx->Yield();
        IM_CHECK_EQ(vars.transport.is_playing(), false);
    };

    t = IM_REGISTER_TEST(engine, "transport", "metronome_toggle");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        IM_CHECK_EQ(vars.transport.metronome_enabled(), false);

        vars.transport.set_metronome_enabled(true);
        ctx->Yield();
        IM_CHECK_EQ(vars.transport.metronome_enabled(), true);

        vars.transport.set_metronome_enabled(false);
        ctx->Yield();
        IM_CHECK_EQ(vars.transport.metronome_enabled(), false);
    };

    t = IM_REGISTER_TEST(engine, "transport", "loop_toggle");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        IM_CHECK_EQ(vars.transport.loop_enabled(), false);

        vars.transport.set_loop_enabled(true);
        ctx->Yield();
        IM_CHECK_EQ(vars.transport.loop_enabled(), true);

        vars.transport.set_loop_enabled(false);
        ctx->Yield();
        IM_CHECK_EQ(vars.transport.loop_enabled(), false);
    };

    t = IM_REGISTER_TEST(engine, "transport", "follow_playhead_toggle");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        bool initial = vars.transport.follow_playhead();
        vars.transport.set_follow_playhead(!initial);
        ctx->Yield();
        IM_CHECK_EQ(vars.transport.follow_playhead(), !initial);

        vars.transport.set_follow_playhead(initial);
        ctx->Yield();
        IM_CHECK_EQ(vars.transport.follow_playhead(), initial);
    };

    t = IM_REGISTER_TEST(engine, "transport", "bpm_control");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        IM_CHECK_EQ(vars.project.tempo().bpm(), 120.0);

        vars.project.tempo().set_bpm(140.0);
        ctx->Yield();
        IM_CHECK_EQ(vars.project.tempo().bpm(), 140.0);

        vars.project.tempo().set_bpm(90.0);
        ctx->Yield();
        IM_CHECK_EQ(vars.project.tempo().bpm(), 90.0);
    };

    t = IM_REGISTER_TEST(engine, "transport", "bpm_clamping");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        vars.project.tempo().set_bpm(0.5);
        ctx->Yield();
        IM_CHECK_GE(vars.project.tempo().bpm(), 1.0);

        vars.project.tempo().set_bpm(1000.0);
        ctx->Yield();
        IM_CHECK_LE(vars.project.tempo().bpm(), 999.0);
    };

    t = IM_REGISTER_TEST(engine, "transport", "grid_subdivision_quarter");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        vars.project.set_grid_subdivision(furious::NoteSubdivision::Quarter);
        ctx->Yield();
        IM_CHECK(vars.project.grid_subdivision() == furious::NoteSubdivision::Quarter);
        double subdiv = vars.project.tempo().subdivision_duration_seconds(furious::NoteSubdivision::Quarter);
        double beat = vars.project.tempo().beat_duration_seconds();
        IM_CHECK_EQ(subdiv, beat);
    };

    t = IM_REGISTER_TEST(engine, "transport", "grid_subdivision_eighth");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        vars.project.set_grid_subdivision(furious::NoteSubdivision::Eighth);
        ctx->Yield();
        IM_CHECK(vars.project.grid_subdivision() == furious::NoteSubdivision::Eighth);
        double subdiv = vars.project.tempo().subdivision_duration_seconds(furious::NoteSubdivision::Eighth);
        double expected = vars.project.tempo().beat_duration_seconds() / 2.0;
        IM_CHECK_EQ(subdiv, expected);
    };

    t = IM_REGISTER_TEST(engine, "transport", "grid_subdivision_sixteenth");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        vars.project.set_grid_subdivision(furious::NoteSubdivision::Sixteenth);
        ctx->Yield();
        IM_CHECK(vars.project.grid_subdivision() == furious::NoteSubdivision::Sixteenth);
        double subdiv = vars.project.tempo().subdivision_duration_seconds(furious::NoteSubdivision::Sixteenth);
        double expected = vars.project.tempo().beat_duration_seconds() / 4.0;
        IM_CHECK_EQ(subdiv, expected);
    };

    t = IM_REGISTER_TEST(engine, "transport", "fps_options");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        double fps_values[] = {24.0, 25.0, 30.0, 50.0, 60.0};
        for (double fps : fps_values) {
            vars.project.set_fps(fps);
            ctx->Yield();
            IM_CHECK_EQ(vars.project.fps(), fps);
        }
    };

    t = IM_REGISTER_TEST(engine, "transport", "beat_duration_at_various_bpm");
    t->SetVarsDataType<TransportTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();
        ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Transport Test")) {
            vars.transport.render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<TransportTestVars>();

        vars.project.tempo().set_bpm(60.0);
        ctx->Yield();
        IM_CHECK_EQ(vars.project.tempo().beat_duration_seconds(), 1.0);

        vars.project.tempo().set_bpm(120.0);
        ctx->Yield();
        IM_CHECK_EQ(vars.project.tempo().beat_duration_seconds(), 0.5);

        vars.project.tempo().set_bpm(180.0);
        ctx->Yield();
        double expected = 60.0 / 180.0;
        double actual = vars.project.tempo().beat_duration_seconds();
        IM_CHECK(std::fabs(actual - expected) < 0.0001);
    };
}
