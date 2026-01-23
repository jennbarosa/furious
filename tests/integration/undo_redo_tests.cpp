#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/core/command.hpp"
#include "furious/core/clip_commands.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/timeline_clip.hpp"
#include "furious/core/project.hpp"
#include "furious/ui/timeline.hpp"

namespace {

struct UndoRedoTestVars {
    furious::Project project;
    furious::TimelineData timeline_data;
    furious::CommandHistory command_history;
    std::unique_ptr<furious::Timeline> timeline;

    UndoRedoTestVars() {
        timeline = std::make_unique<furious::Timeline>(project);
        timeline->set_timeline_data(&timeline_data);
    }
};

furious::TimelineClip make_test_clip(const std::string& id, size_t track, double start, double duration) {
    furious::TimelineClip clip;
    clip.id = id;
    clip.source_id = "test-source";
    clip.track_index = track;
    clip.start_beat = start;
    clip.duration_beats = duration;
    clip.position_x = 0.0f;
    clip.position_y = 0.0f;
    clip.scale_x = 1.0f;
    clip.scale_y = 1.0f;
    clip.rotation = 0.0f;
    return clip;
}

}

void RegisterUndoRedoTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "undo_redo", "add_clip_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 0u);
        IM_CHECK_EQ(vars.command_history.can_undo(), false);

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(vars.timeline_data, clip));

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);
        IM_CHECK_EQ(vars.command_history.can_undo(), true);
        IM_CHECK_EQ(vars.command_history.can_redo(), false);

        vars.command_history.undo();

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 0u);
        IM_CHECK_EQ(vars.command_history.can_undo(), false);
        IM_CHECK_EQ(vars.command_history.can_redo(), true);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "add_clip_redo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(vars.timeline_data, clip));
        vars.command_history.undo();

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 0u);
        IM_CHECK_EQ(vars.command_history.can_redo(), true);

        vars.command_history.redo();

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);
        IM_CHECK(vars.timeline_data.find_clip("clip-1") != nullptr);
        IM_CHECK_EQ(vars.command_history.can_undo(), true);
        IM_CHECK_EQ(vars.command_history.can_redo(), false);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "remove_clip_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        vars.timeline_data.add_clip(make_test_clip("clip-1", 0, 0.0, 4.0));
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);

        vars.command_history.execute(std::make_unique<furious::RemoveClipCommand>(vars.timeline_data, "clip-1"));

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 0u);

        vars.command_history.undo();

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);
        IM_CHECK(vars.timeline_data.find_clip("clip-1") != nullptr);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "modify_clip_position_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        clip.position_x = 0.0f;
        clip.position_y = 0.0f;
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;

        c->position_x = 100.0f;
        c->position_y = 50.0f;
        furious::TimelineClip new_state = *c;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, new_state, "Move clip"));

        IM_CHECK_EQ(c->position_x, 100.0f);
        IM_CHECK_EQ(c->position_y, 50.0f);

        vars.command_history.undo();

        IM_CHECK_EQ(c->position_x, 0.0f);
        IM_CHECK_EQ(c->position_y, 0.0f);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "modify_clip_scale_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;

        c->scale_x = 2.0f;
        c->scale_y = 0.5f;
        furious::TimelineClip new_state = *c;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, new_state, "Scale clip"));

        IM_CHECK_EQ(c->scale_x, 2.0f);
        IM_CHECK_EQ(c->scale_y, 0.5f);

        vars.command_history.undo();

        IM_CHECK_EQ(c->scale_x, 1.0f);
        IM_CHECK_EQ(c->scale_y, 1.0f);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "modify_clip_rotation_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;

        c->rotation = 45.0f;
        furious::TimelineClip new_state = *c;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, new_state, "Rotate clip"));

        IM_CHECK_EQ(c->rotation, 45.0f);

        vars.command_history.undo();

        IM_CHECK_EQ(c->rotation, 0.0f);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "modify_clip_timeline_position_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;

        c->start_beat = 8.0;
        furious::TimelineClip new_state = *c;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, new_state, "Move clip on timeline"));

        ctx->Yield();
        IM_CHECK_EQ(c->start_beat, 8.0);

        vars.command_history.undo();

        ctx->Yield();
        IM_CHECK_EQ(c->start_beat, 0.0);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "effect_enable_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        IM_CHECK_EQ(c->effects.size(), 0u);

        furious::TimelineClip old_state = *c;

        furious::ClipEffect effect;
        effect.effect_id = "test_effect";
        effect.enabled = true;
        effect.parameters["param1"] = "value1";
        c->effects.push_back(effect);

        furious::TimelineClip new_state = *c;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, new_state, "Enable effect"));

        IM_CHECK_EQ(c->effects.size(), 1u);

        vars.command_history.undo();

        IM_CHECK_EQ(c->effects.size(), 0u);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "effect_disable_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        furious::ClipEffect effect;
        effect.effect_id = "test_effect";
        effect.enabled = true;
        clip.effects.push_back(effect);
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        IM_CHECK_EQ(c->effects.size(), 1u);

        furious::TimelineClip old_state = *c;
        c->effects.clear();
        furious::TimelineClip new_state = *c;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, new_state, "Disable effect"));

        IM_CHECK_EQ(c->effects.size(), 0u);

        vars.command_history.undo();

        IM_CHECK_EQ(c->effects.size(), 1u);
        IM_CHECK_EQ(c->effects[0].effect_id, "test_effect");
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "effect_parameter_change_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        furious::ClipEffect effect;
        effect.effect_id = "test_effect";
        effect.enabled = true;
        effect.parameters["value"] = "10.0";
        clip.effects.push_back(effect);
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;

        c->effects[0].parameters["value"] = "50.0";
        furious::TimelineClip new_state = *c;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, new_state, "Change effect parameter"));

        IM_CHECK_EQ(c->effects[0].parameters["value"], "50.0");

        vars.command_history.undo();

        IM_CHECK_EQ(c->effects[0].parameters["value"], "10.0");
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "multiple_undo_redo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-1", 0, 0.0, 4.0)));
        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-2", 0, 4.0, 4.0)));
        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-3", 0, 8.0, 4.0)));

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 3u);

        vars.command_history.undo();
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 2u);
        IM_CHECK(vars.timeline_data.find_clip("clip-3") == nullptr);

        vars.command_history.undo();
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);
        IM_CHECK(vars.timeline_data.find_clip("clip-2") == nullptr);

        vars.command_history.undo();
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 0u);

        vars.command_history.redo();
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);
        IM_CHECK(vars.timeline_data.find_clip("clip-1") != nullptr);

        vars.command_history.redo();
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 2u);

        vars.command_history.redo();
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 3u);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "new_action_clears_redo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-1", 0, 0.0, 4.0)));
        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-2", 0, 4.0, 4.0)));

        vars.command_history.undo();
        IM_CHECK_EQ(vars.command_history.can_redo(), true);

        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-3", 0, 8.0, 4.0)));

        IM_CHECK_EQ(vars.command_history.can_redo(), false);
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 2u);
        IM_CHECK(vars.timeline_data.find_clip("clip-1") != nullptr);
        IM_CHECK(vars.timeline_data.find_clip("clip-3") != nullptr);
        IM_CHECK(vars.timeline_data.find_clip("clip-2") == nullptr);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "clear_history");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-1", 0, 0.0, 4.0)));
        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-2", 0, 4.0, 4.0)));

        vars.command_history.undo();

        IM_CHECK_EQ(vars.command_history.can_undo(), true);
        IM_CHECK_EQ(vars.command_history.can_redo(), true);

        vars.command_history.clear();

        IM_CHECK_EQ(vars.command_history.can_undo(), false);
        IM_CHECK_EQ(vars.command_history.can_redo(), false);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "undo_description");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        IM_CHECK_EQ(vars.command_history.undo_description(), "");

        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-1", 0, 0.0, 4.0)));

        IM_CHECK_EQ(vars.command_history.undo_description(), "Add clip");

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;
        c->position_x = 100.0f;
        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, *c, "Move clip in viewport"));

        IM_CHECK_EQ(vars.command_history.undo_description(), "Move clip in viewport");
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "transform_all_properties_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 4.0);
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;

        c->position_x = 100.0f;
        c->position_y = -50.0f;
        c->scale_x = 2.0f;
        c->scale_y = 0.5f;
        c->rotation = 90.0f;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, *c, "Reset all transforms"));

        IM_CHECK_EQ(c->position_x, 100.0f);
        IM_CHECK_EQ(c->position_y, -50.0f);
        IM_CHECK_EQ(c->scale_x, 2.0f);
        IM_CHECK_EQ(c->scale_y, 0.5f);
        IM_CHECK_EQ(c->rotation, 90.0f);

        vars.command_history.undo();

        IM_CHECK_EQ(c->position_x, 0.0f);
        IM_CHECK_EQ(c->position_y, 0.0f);
        IM_CHECK_EQ(c->scale_x, 1.0f);
        IM_CHECK_EQ(c->scale_y, 1.0f);
        IM_CHECK_EQ(c->rotation, 0.0f);

        vars.command_history.redo();

        IM_CHECK_EQ(c->position_x, 100.0f);
        IM_CHECK_EQ(c->position_y, -50.0f);
        IM_CHECK_EQ(c->scale_x, 2.0f);
        IM_CHECK_EQ(c->scale_y, 0.5f);
        IM_CHECK_EQ(c->rotation, 90.0f);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "clip_trim_undo");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();
        ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Timeline Test")) {
            vars.timeline->render();
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        auto clip = make_test_clip("clip-1", 0, 0.0, 8.0);
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;

        c->duration_beats = 4.0;
        furious::TimelineClip new_state = *c;

        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, new_state, "Trim clip"));

        ctx->Yield();
        IM_CHECK_EQ(c->duration_beats, 4.0);

        vars.command_history.undo();

        ctx->Yield();
        IM_CHECK_EQ(c->duration_beats, 8.0);
    };

    t = IM_REGISTER_TEST(engine, "undo_redo", "mixed_operations_sequence");
    t->SetVarsDataType<UndoRedoTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Undo Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UndoRedoTestVars>();

        vars.command_history.execute(std::make_unique<furious::AddClipCommand>(
            vars.timeline_data, make_test_clip("clip-1", 0, 0.0, 4.0)));
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);

        auto* c = vars.timeline_data.find_clip("clip-1");
        furious::TimelineClip old_state = *c;
        c->position_x = 100.0f;
        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, *c, "Move clip"));
        IM_CHECK_EQ(c->position_x, 100.0f);

        old_state = *c;
        furious::ClipEffect effect;
        effect.effect_id = "test";
        effect.enabled = true;
        c->effects.push_back(effect);
        vars.command_history.execute(std::make_unique<furious::ModifyClipCommand>(
            vars.timeline_data, "clip-1", old_state, *c, "Enable effect"));
        IM_CHECK_EQ(c->effects.size(), 1u);

        vars.command_history.undo();
        IM_CHECK_EQ(c->effects.size(), 0u);
        IM_CHECK_EQ(c->position_x, 100.0f);

        vars.command_history.undo();
        IM_CHECK_EQ(c->position_x, 0.0f);

        vars.command_history.undo();
        IM_CHECK_EQ(vars.timeline_data.clips().size(), 0u);

        vars.command_history.redo();
        vars.command_history.redo();
        vars.command_history.redo();

        c = vars.timeline_data.find_clip("clip-1");
        IM_CHECK(c != nullptr);
        IM_CHECK_EQ(c->position_x, 100.0f);
        IM_CHECK_EQ(c->effects.size(), 1u);
    };
}
