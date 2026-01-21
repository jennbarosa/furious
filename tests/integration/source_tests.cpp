#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/video/source_library.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/media_source.hpp"

namespace {

struct SourceTestVars {
    furious::SourceLibrary library;
    furious::TimelineData timeline_data;
};

furious::MediaSource make_source(const std::string& id, const std::string& name, furious::MediaType type) {
    furious::MediaSource source;
    source.id = id;
    source.name = name;
    source.type = type;
    return source;
}

}

void RegisterSourceTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "sources", "add_source");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        IM_CHECK_EQ(vars.library.sources().size(), 0u);

        furious::MediaSource source;
        source.id = "test-source-1";
        source.name = "Test Video";
        source.filepath = "/path/to/video.mp4";
        source.type = furious::MediaType::Video;
        source.duration_seconds = 10.0;
        source.width = 1920;
        source.height = 1080;
        source.fps = 30.0;
        vars.library.add_source_direct(source);

        IM_CHECK_EQ(vars.library.sources().size(), 1u);
        IM_CHECK(vars.library.find_source("test-source-1") != nullptr);
    };

    t = IM_REGISTER_TEST(engine, "sources", "remove_source");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        vars.library.add_source_direct(make_source("removable-source", "To Remove", furious::MediaType::Video));

        IM_CHECK_EQ(vars.library.sources().size(), 1u);

        vars.library.remove_source("removable-source");
        IM_CHECK_EQ(vars.library.sources().size(), 0u);
        IM_CHECK(vars.library.find_source("removable-source") == nullptr);
    };

    t = IM_REGISTER_TEST(engine, "sources", "source_types");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        vars.library.add_source_direct(make_source("video-source", "Video", furious::MediaType::Video));
        vars.library.add_source_direct(make_source("image-source", "Image", furious::MediaType::Image));

        auto* v = vars.library.find_source("video-source");
        auto* i = vars.library.find_source("image-source");

        IM_CHECK(v != nullptr);
        IM_CHECK(i != nullptr);
        IM_CHECK(v->type == furious::MediaType::Video);
        IM_CHECK(i->type == furious::MediaType::Image);
    };

    t = IM_REGISTER_TEST(engine, "sources", "source_metadata");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        furious::MediaSource source;
        source.id = "metadata-test";
        source.name = "4K Video";
        source.filepath = "/videos/4k.mp4";
        source.type = furious::MediaType::Video;
        source.duration_seconds = 120.5;
        source.width = 3840;
        source.height = 2160;
        source.fps = 60.0;
        vars.library.add_source_direct(source);

        auto* s = vars.library.find_source("metadata-test");
        IM_CHECK(s != nullptr);
        IM_CHECK_EQ(s->name, std::string("4K Video"));
        IM_CHECK_EQ(s->duration_seconds, 120.5);
        IM_CHECK_EQ(s->width, 3840);
        IM_CHECK_EQ(s->height, 2160);
        IM_CHECK_EQ(s->fps, 60.0);
    };

    t = IM_REGISTER_TEST(engine, "sources", "source_to_clip");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        furious::MediaSource source;
        source.id = "clip-source";
        source.name = "Clip Source";
        source.type = furious::MediaType::Video;
        source.duration_seconds = 5.0;
        vars.library.add_source_direct(source);

        furious::TimelineClip clip;
        clip.id = "new-clip";
        clip.source_id = "clip-source";
        clip.track_index = 0;
        clip.start_beat = 4.0;
        clip.duration_beats = 8.0;
        vars.timeline_data.add_clip(clip);

        auto* c = vars.timeline_data.find_clip("new-clip");
        IM_CHECK(c != nullptr);
        IM_CHECK_EQ(c->source_id, std::string("clip-source"));
        IM_CHECK_EQ(c->start_beat, 4.0);
        IM_CHECK_EQ(c->duration_beats, 8.0);
    };

    t = IM_REGISTER_TEST(engine, "sources", "remove_clips_by_source");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        furious::TimelineClip clip1;
        clip1.id = "clip-a";
        clip1.source_id = "source-x";
        vars.timeline_data.add_clip(clip1);

        furious::TimelineClip clip2;
        clip2.id = "clip-b";
        clip2.source_id = "source-x";
        vars.timeline_data.add_clip(clip2);

        furious::TimelineClip clip3;
        clip3.id = "clip-c";
        clip3.source_id = "source-y";
        vars.timeline_data.add_clip(clip3);

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 3u);

        vars.timeline_data.remove_clips_by_source("source-x");

        IM_CHECK_EQ(vars.timeline_data.clips().size(), 1u);
        IM_CHECK(vars.timeline_data.find_clip("clip-a") == nullptr);
        IM_CHECK(vars.timeline_data.find_clip("clip-b") == nullptr);
        IM_CHECK(vars.timeline_data.find_clip("clip-c") != nullptr);
    };

    t = IM_REGISTER_TEST(engine, "sources", "multiple_sources");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        for (int i = 0; i < 10; ++i) {
            furious::MediaSource source;
            source.id = "source-" + std::to_string(i);
            source.name = "Video " + std::to_string(i);
            source.type = furious::MediaType::Video;
            vars.library.add_source_direct(source);
        }

        IM_CHECK_EQ(vars.library.sources().size(), 10u);

        for (int i = 0; i < 10; ++i) {
            std::string id = "source-" + std::to_string(i);
            IM_CHECK(vars.library.find_source(id) != nullptr);
        }
    };

    t = IM_REGISTER_TEST(engine, "sources", "clear_library");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        for (int i = 0; i < 5; ++i) {
            furious::MediaSource source;
            source.id = "src-" + std::to_string(i);
            vars.library.add_source_direct(source);
        }

        IM_CHECK_EQ(vars.library.sources().size(), 5u);

        vars.library.clear();
        IM_CHECK_EQ(vars.library.sources().size(), 0u);
    };

    t = IM_REGISTER_TEST(engine, "sources", "has_clips_using_source_returns_false_when_no_clips");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        vars.library.add_source_direct(make_source("unused-source", "Unused", furious::MediaType::Video));

        IM_CHECK_EQ(vars.timeline_data.has_clips_using_source("unused-source"), false);
    };

    t = IM_REGISTER_TEST(engine, "sources", "has_clips_using_source_returns_true_when_clips_exist");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        vars.library.add_source_direct(make_source("used-source", "Used", furious::MediaType::Video));

        furious::TimelineClip clip;
        clip.id = "clip-using-source";
        clip.source_id = "used-source";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 4.0;
        vars.timeline_data.add_clip(clip);

        IM_CHECK_EQ(vars.timeline_data.has_clips_using_source("used-source"), true);
        IM_CHECK_EQ(vars.timeline_data.has_clips_using_source("other-source"), false);
    };

    t = IM_REGISTER_TEST(engine, "sources", "has_clips_using_source_returns_false_after_clips_removed");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        furious::TimelineClip clip;
        clip.id = "removable-clip";
        clip.source_id = "source-to-check";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 4.0;
        vars.timeline_data.add_clip(clip);

        IM_CHECK_EQ(vars.timeline_data.has_clips_using_source("source-to-check"), true);

        vars.timeline_data.remove_clip("removable-clip");

        IM_CHECK_EQ(vars.timeline_data.has_clips_using_source("source-to-check"), false);
    };

    t = IM_REGISTER_TEST(engine, "sources", "find_available_track_returns_first_empty_track");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        size_t track = vars.timeline_data.find_available_track(0.0, 4.0);
        IM_CHECK_EQ(track, 0u);
    };

    t = IM_REGISTER_TEST(engine, "sources", "find_available_track_skips_occupied_track");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        vars.timeline_data.add_track();

        furious::TimelineClip clip;
        clip.id = "blocking-clip";
        clip.source_id = "src";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 8.0;
        vars.timeline_data.add_clip(clip);

        size_t track = vars.timeline_data.find_available_track(2.0, 4.0);
        IM_CHECK_EQ(track, 1u);
    };

    t = IM_REGISTER_TEST(engine, "sources", "find_available_track_returns_new_track_index_when_all_occupied");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        furious::TimelineClip clip1;
        clip1.id = "clip-track-0";
        clip1.source_id = "src";
        clip1.track_index = 0;
        clip1.start_beat = 0.0;
        clip1.duration_beats = 8.0;
        vars.timeline_data.add_clip(clip1);

        size_t track = vars.timeline_data.find_available_track(2.0, 4.0);
        IM_CHECK_EQ(track, vars.timeline_data.track_count());
    };

    t = IM_REGISTER_TEST(engine, "sources", "find_available_track_allows_adjacent_clips");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        furious::TimelineClip clip;
        clip.id = "existing-clip";
        clip.source_id = "src";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 4.0;
        vars.timeline_data.add_clip(clip);

        size_t track = vars.timeline_data.find_available_track(4.0, 4.0);
        IM_CHECK_EQ(track, 0u);
    };

    t = IM_REGISTER_TEST(engine, "sources", "find_available_track_detects_partial_overlap");
    t->SetVarsDataType<SourceTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Source Test");
        ImGui::Text("Source Library Tests");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<SourceTestVars>();

        vars.timeline_data.add_track();

        furious::TimelineClip clip;
        clip.id = "existing-clip";
        clip.source_id = "src";
        clip.track_index = 0;
        clip.start_beat = 4.0;
        clip.duration_beats = 4.0;
        vars.timeline_data.add_clip(clip);

        size_t track = vars.timeline_data.find_available_track(2.0, 4.0);
        IM_CHECK_EQ(track, 1u);

        size_t track2 = vars.timeline_data.find_available_track(6.0, 4.0);
        IM_CHECK_EQ(track2, 1u);
    };
}
