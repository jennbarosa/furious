#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/core/pattern.hpp"
#include "furious/core/pattern_library.hpp"
#include "furious/core/pattern_evaluator.hpp"
#include "furious/core/timeline_clip.hpp"
#include "furious/core/project.hpp"
#include "furious/core/timeline_data.hpp"
#include <cmath>

namespace {

struct PatternTestVars {
    furious::PatternLibrary library;
    furious::PatternEvaluator evaluator;
    furious::TimelineData timeline_data;

    PatternTestVars() {
        evaluator.set_pattern_library(&library);
    }
};

}

void RegisterPatternTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "pattern", "library_create_pattern");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Test Pattern");
        IM_CHECK(!id.empty());
        IM_CHECK_EQ(vars.library.pattern_count(), 1u);

        const furious::Pattern* p = vars.library.find_pattern(id);
        IM_CHECK(p != nullptr);
        IM_CHECK_STR_EQ(p->name.c_str(), "Test Pattern");
        IM_CHECK_EQ(p->length_subdivisions, 16);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "library_multiple_patterns");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id1 = vars.library.create_pattern("Pattern 1");
        std::string id2 = vars.library.create_pattern("Pattern 2");
        std::string id3 = vars.library.create_pattern("Pattern 3");

        IM_CHECK_EQ(vars.library.pattern_count(), 3u);
        IM_CHECK(vars.library.find_pattern(id1) != nullptr);
        IM_CHECK(vars.library.find_pattern(id2) != nullptr);
        IM_CHECK(vars.library.find_pattern(id3) != nullptr);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "evaluator_basic_trigger");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Scale Pattern");
        furious::Pattern* p = vars.library.find_pattern(id);
        p->triggers.push_back({0, furious::PatternTargetProperty::ScaleX, 2.0f});

        furious::TimelineClip clip;
        clip.id = "test-clip";
        clip.patterns.push_back({id, true, 0});

        auto result = vars.evaluator.evaluate(clip, 0.0);

        IM_CHECK(result.scale_x.has_value());
        IM_CHECK(std::fabs(*result.scale_x - 2.0f) < 0.001f);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "evaluator_flip_trigger");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Flip Pattern");
        furious::Pattern* p = vars.library.find_pattern(id);
        p->triggers.push_back({0, furious::PatternTargetProperty::FlipH, 1.0f});
        p->triggers.push_back({8, furious::PatternTargetProperty::FlipH, 0.0f});

        furious::TimelineClip clip;
        clip.id = "flip-clip";
        clip.patterns.push_back({id, true, 0});

        auto result = vars.evaluator.evaluate(clip, 0.0);
        IM_CHECK(result.flip_h.has_value());
        IM_CHECK_EQ(*result.flip_h, true);

        result = vars.evaluator.evaluate(clip, 2.0);
        IM_CHECK(result.flip_h.has_value());
        IM_CHECK_EQ(*result.flip_h, false);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "evaluator_pattern_wrapping");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Wrap Pattern");
        furious::Pattern* p = vars.library.find_pattern(id);
        p->length_subdivisions = 8;
        p->triggers.push_back({0, furious::PatternTargetProperty::ScaleX, 1.0f});
        p->triggers.push_back({4, furious::PatternTargetProperty::ScaleX, 2.0f});

        furious::TimelineClip clip;
        clip.id = "wrap-clip";
        clip.patterns.push_back({id, true, 0});

        auto result = vars.evaluator.evaluate(clip, 0.0);
        IM_CHECK(std::fabs(*result.scale_x - 1.0f) < 0.001f);

        result = vars.evaluator.evaluate(clip, 1.0);
        IM_CHECK(std::fabs(*result.scale_x - 2.0f) < 0.001f);

        result = vars.evaluator.evaluate(clip, 2.0);
        IM_CHECK(std::fabs(*result.scale_x - 1.0f) < 0.001f);

        result = vars.evaluator.evaluate(clip, 3.0);
        IM_CHECK(std::fabs(*result.scale_x - 2.0f) < 0.001f);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "evaluator_offset");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Offset Pattern");
        furious::Pattern* p = vars.library.find_pattern(id);
        p->triggers.push_back({4, furious::PatternTargetProperty::ScaleX, 3.0f});

        furious::TimelineClip clip;
        clip.id = "offset-clip";
        clip.patterns.push_back({id, true, 4});

        auto result = vars.evaluator.evaluate(clip, 0.0);
        IM_CHECK(std::fabs(*result.scale_x - 3.0f) < 0.001f);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "evaluator_disabled_pattern");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Disabled");
        furious::Pattern* p = vars.library.find_pattern(id);
        p->triggers.push_back({0, furious::PatternTargetProperty::ScaleX, 5.0f});

        furious::TimelineClip clip;
        clip.id = "disabled-clip";
        clip.patterns.push_back({id, false, 0});

        auto result = vars.evaluator.evaluate(clip, 0.0);
        IM_CHECK(!result.scale_x.has_value());
    };

    t = IM_REGISTER_TEST(engine, "pattern", "evaluator_multiple_properties");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Multi");
        furious::Pattern* p = vars.library.find_pattern(id);
        p->triggers.push_back({0, furious::PatternTargetProperty::PositionX, 100.0f});
        p->triggers.push_back({0, furious::PatternTargetProperty::PositionY, -50.0f});
        p->triggers.push_back({0, furious::PatternTargetProperty::ScaleX, 2.0f});
        p->triggers.push_back({0, furious::PatternTargetProperty::ScaleY, 0.5f});
        p->triggers.push_back({0, furious::PatternTargetProperty::Rotation, 45.0f});
        p->triggers.push_back({0, furious::PatternTargetProperty::FlipH, 1.0f});
        p->triggers.push_back({0, furious::PatternTargetProperty::FlipV, 1.0f});

        furious::TimelineClip clip;
        clip.id = "multi-clip";
        clip.patterns.push_back({id, true, 0});

        auto result = vars.evaluator.evaluate(clip, 0.0);

        IM_CHECK(std::fabs(*result.position_x - 100.0f) < 0.001f);
        IM_CHECK(std::fabs(*result.position_y - (-50.0f)) < 0.001f);
        IM_CHECK(std::fabs(*result.scale_x - 2.0f) < 0.001f);
        IM_CHECK(std::fabs(*result.scale_y - 0.5f) < 0.001f);
        IM_CHECK(std::fabs(*result.rotation - 45.0f) < 0.001f);
        IM_CHECK_EQ(*result.flip_h, true);
        IM_CHECK_EQ(*result.flip_v, true);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "evaluator_held_value_persists");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Held");
        furious::Pattern* p = vars.library.find_pattern(id);
        p->triggers.push_back({0, furious::PatternTargetProperty::ScaleX, 2.0f});

        furious::TimelineClip clip;
        clip.id = "held-clip";
        clip.patterns.push_back({id, true, 0});

        for (double beat = 0.0; beat < 4.0; beat += 0.25) {
            auto result = vars.evaluator.evaluate(clip, beat);
            IM_CHECK(result.scale_x.has_value());
            IM_CHECK(std::fabs(*result.scale_x - 2.0f) < 0.001f);
        }
    };

    t = IM_REGISTER_TEST(engine, "pattern", "library_duplicate");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string original_id = vars.library.create_pattern("Original");
        furious::Pattern* original = vars.library.find_pattern(original_id);
        original->triggers.push_back({0, furious::PatternTargetProperty::ScaleX, 3.0f});
        original->triggers.push_back({8, furious::PatternTargetProperty::FlipH, 1.0f});

        std::string copy_id = vars.library.duplicate_pattern(original_id);

        IM_CHECK(!copy_id.empty());
        IM_CHECK(copy_id != original_id);
        IM_CHECK_EQ(vars.library.pattern_count(), 2u);

        const furious::Pattern* copy = vars.library.find_pattern(copy_id);
        IM_CHECK_EQ(copy->triggers.size(), 2u);
        IM_CHECK(std::fabs(copy->triggers[0].value - 3.0f) < 0.001f);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "library_remove");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id1 = vars.library.create_pattern("Keep");
        std::string id2 = vars.library.create_pattern("Remove");

        IM_CHECK_EQ(vars.library.pattern_count(), 2u);

        vars.library.remove_pattern(id2);

        IM_CHECK_EQ(vars.library.pattern_count(), 1u);
        IM_CHECK(vars.library.find_pattern(id1) != nullptr);
        IM_CHECK(vars.library.find_pattern(id2) == nullptr);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "clip_with_patterns_workflow");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string flip_id = vars.library.create_pattern("Flip Every Beat");
        furious::Pattern* flip_pattern = vars.library.find_pattern(flip_id);
        flip_pattern->triggers.push_back({0, furious::PatternTargetProperty::FlipH, 1.0f});
        flip_pattern->triggers.push_back({4, furious::PatternTargetProperty::FlipH, 0.0f});
        flip_pattern->triggers.push_back({8, furious::PatternTargetProperty::FlipH, 1.0f});
        flip_pattern->triggers.push_back({12, furious::PatternTargetProperty::FlipH, 0.0f});

        std::string scale_id = vars.library.create_pattern("Pulse Scale");
        furious::Pattern* scale_pattern = vars.library.find_pattern(scale_id);
        scale_pattern->triggers.push_back({0, furious::PatternTargetProperty::ScaleX, 1.5f});
        scale_pattern->triggers.push_back({0, furious::PatternTargetProperty::ScaleY, 1.5f});
        scale_pattern->triggers.push_back({2, furious::PatternTargetProperty::ScaleX, 1.0f});
        scale_pattern->triggers.push_back({2, furious::PatternTargetProperty::ScaleY, 1.0f});

        furious::TimelineClip clip;
        clip.id = "workflow-clip";
        clip.source_id = "video_source";
        clip.start_beat = 0.0;
        clip.duration_beats = 8.0;
        clip.patterns.push_back({flip_id, true, 0});
        clip.patterns.push_back({scale_id, true, 0});

        vars.timeline_data.add_clip(clip);

        auto result = vars.evaluator.evaluate(clip, 0.0);
        IM_CHECK_EQ(*result.flip_h, true);
        IM_CHECK(std::fabs(*result.scale_x - 1.5f) < 0.001f);

        result = vars.evaluator.evaluate(clip, 0.5);
        IM_CHECK_EQ(*result.flip_h, true);
        IM_CHECK(std::fabs(*result.scale_x - 1.0f) < 0.001f);

        result = vars.evaluator.evaluate(clip, 1.0);
        IM_CHECK_EQ(*result.flip_h, false);
        IM_CHECK(std::fabs(*result.scale_x - 1.0f) < 0.001f);
    };

    t = IM_REGISTER_TEST(engine, "pattern", "fractional_beat_evaluation");
    t->SetVarsDataType<PatternTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Pattern Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<PatternTestVars>();

        std::string id = vars.library.create_pattern("Sixteenth");
        furious::Pattern* p = vars.library.find_pattern(id);
        for (int i = 0; i < 16; ++i) {
            p->triggers.push_back({i, furious::PatternTargetProperty::ScaleX, static_cast<float>(i + 1)});
        }

        furious::TimelineClip clip;
        clip.id = "sixteenth-clip";
        clip.patterns.push_back({id, true, 0});

        for (int i = 0; i < 16; ++i) {
            double beat = i / 4.0;
            auto result = vars.evaluator.evaluate(clip, beat);
            IM_CHECK(result.scale_x.has_value());
            IM_CHECK(std::fabs(*result.scale_x - static_cast<float>(i + 1)) < 0.001f);
        }
    };
}
