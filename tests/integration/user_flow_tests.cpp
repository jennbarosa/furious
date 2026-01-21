#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/ui/main_window.hpp"
#include "furious/core/project_data.hpp"
#include <cmath>
#include <filesystem>

namespace {

struct UserFlowTestVars {
    std::unique_ptr<furious::MainWindow> main_window;
    std::string test_project_path;
    std::string dummy_source_id;

    UserFlowTestVars() {
        main_window = std::make_unique<furious::MainWindow>();
        test_project_path = "/tmp/furious_test_project.furious";
    }

    ~UserFlowTestVars() {
        if (std::filesystem::exists(test_project_path)) {
            std::filesystem::remove(test_project_path);
        }
    }
};

furious::MediaSource create_dummy_source(const std::string& id, const std::string& name) {
    furious::MediaSource source;
    source.id = id;
    source.name = name;
    source.filepath = "/dummy/path/video.mp4";
    source.type = furious::MediaType::Video;
    source.duration_seconds = 10.0;
    source.width = 1920;
    source.height = 1080;
    source.fps = 30.0;
    return source;
}

}

void RegisterUserFlowTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "user_flow", "complete_workflow");
    t->SetVarsDataType<UserFlowTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        vars.main_window->render();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        auto& mw = *vars.main_window;

        mw.project().set_name("Test Project");
        mw.project().tempo().set_bpm(140.0);
        IM_CHECK_EQ(mw.project().tempo().bpm(), 140.0);

        bool saved = mw.save_project(vars.test_project_path);
        IM_CHECK(saved);
        ctx->Yield();

        mw.project().set_name("Different Name");
        mw.project().tempo().set_bpm(100.0);

        bool loaded = mw.load_project(vars.test_project_path);
        IM_CHECK(loaded);
        IM_CHECK_EQ(mw.project().name(), std::string("Test Project"));
        IM_CHECK_EQ(mw.project().tempo().bpm(), 140.0);
        ctx->Yield();
        IM_CHECK_EQ(mw.audio_engine().has_clip(), false);

        ctx->Yield();

        mw.audio_engine().set_clip_start_seconds(1.0);
        mw.audio_engine().set_clip_end_seconds(5.0);
        IM_CHECK_EQ(mw.audio_engine().clip_start_seconds(), 1.0);
        IM_CHECK_EQ(mw.audio_engine().clip_end_seconds(), 5.0);

        double trimmed = mw.audio_engine().trimmed_duration_seconds();
        IM_CHECK_EQ(trimmed, 4.0);
        ctx->Yield();

        mw.audio_engine().reset_clip_bounds();
        IM_CHECK_EQ(mw.audio_engine().clip_start_seconds(), 0.0);
        IM_CHECK_EQ(mw.audio_engine().clip_end_seconds(), 0.0);
        ctx->Yield();

        IM_CHECK_EQ(mw.source_library().sources().size(), 0u);

        auto dummy_source = create_dummy_source("dummy-source-1", "Test Video");
        mw.source_library().add_source_direct(dummy_source);
        vars.dummy_source_id = "dummy-source-1";

        IM_CHECK_EQ(mw.source_library().sources().size(), 1u);
        IM_CHECK(mw.source_library().find_source("dummy-source-1") != nullptr);
        ctx->Yield();

        furious::TimelineClip clip;
        clip.id = "test-clip-1";
        clip.source_id = vars.dummy_source_id;
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 8.0;
        mw.timeline_data().add_clip(clip);

        IM_CHECK_EQ(mw.timeline_data().clips().size(), 1u);
        auto* added_clip = mw.timeline_data().find_clip("test-clip-1");
        IM_CHECK(added_clip != nullptr);
        IM_CHECK_EQ(added_clip->start_beat, 0.0);
        IM_CHECK_EQ(added_clip->duration_beats, 8.0);
        ctx->Yield();

        mw.transport_controls().set_playing(true);
        IM_CHECK(mw.transport_controls().is_playing());

        mw.timeline().set_playhead_position(0.0);
        IM_CHECK_EQ(mw.timeline().playhead_position(), 0.0);

        mw.timeline().update(0.5, true);
        IM_CHECK_GT(mw.timeline().playhead_position(), 0.0);

        mw.timeline().set_playhead_position(4.0);
        IM_CHECK_EQ(mw.timeline().playhead_position(), 4.0);

        auto clips_at_4 = mw.timeline_data().clips_at_beat(4.0);
        IM_CHECK_EQ(clips_at_4.size(), 1u);

        mw.transport_controls().set_playing(false);
        IM_CHECK_EQ(mw.transport_controls().is_playing(), false);
        ctx->Yield();

        size_t initial_tracks = mw.timeline_data().track_count();

        mw.timeline_data().add_track("Track 2");
        mw.timeline_data().add_track("Track 3");
        mw.timeline_data().add_track("Track 4");
        mw.timeline_data().add_track("Track 5");
        mw.timeline_data().add_track("Track 6");

        IM_CHECK_EQ(mw.timeline_data().track_count(), initial_tracks + 5);
        ctx->Yield();

        mw.timeline_data().remove_track(5);
        mw.timeline_data().remove_track(4);
        mw.timeline_data().remove_track(3);

        IM_CHECK_EQ(mw.timeline_data().track_count(), initial_tracks + 2);
        ctx->Yield();

        mw.timeline_data().add_track("New Track");
        IM_CHECK_EQ(mw.timeline_data().track_count(), initial_tracks + 3);
        ctx->Yield();

        added_clip = mw.timeline_data().find_clip("test-clip-1");
        IM_CHECK(added_clip != nullptr);
        IM_CHECK_EQ(added_clip->track_index, 0u);

        added_clip->track_index = 1;
        ctx->Yield();

        IM_CHECK_EQ(added_clip->track_index, 1u);

        auto track1_clips = mw.timeline_data().clips_on_track(1);
        IM_CHECK_EQ(track1_clips.size(), 1u);
        ctx->Yield();

        size_t before_delete = mw.timeline_data().track_count();
        mw.timeline_data().remove_track(mw.timeline_data().track_count() - 1);
        IM_CHECK_EQ(mw.timeline_data().track_count(), before_delete - 1);
        ctx->Yield();

        added_clip->track_index = 0;
        IM_CHECK_EQ(added_clip->track_index, 0u);

        auto track0_clips = mw.timeline_data().clips_on_track(0);
        IM_CHECK_EQ(track0_clips.size(), 1u);
        ctx->Yield();

        mw.timeline().set_selected_clip_id("test-clip-1");
        IM_CHECK_EQ(mw.timeline().selected_clip_id(), std::string("test-clip-1"));
        ctx->Yield();

        added_clip = mw.timeline_data().find_clip("test-clip-1");
        IM_CHECK(added_clip != nullptr);

        IM_CHECK_EQ(added_clip->position_x, 0.0f);
        IM_CHECK_EQ(added_clip->position_y, 0.0f);

        added_clip->position_x = -10000.0f;
        IM_CHECK_EQ(added_clip->position_x, -10000.0f);

        added_clip->position_x = 10000.0f;
        IM_CHECK_EQ(added_clip->position_x, 10000.0f);

        added_clip->position_x = 500.0f;
        added_clip->position_y = -200.0f;
        IM_CHECK_EQ(added_clip->position_x, 500.0f);
        IM_CHECK_EQ(added_clip->position_y, -200.0f);

        added_clip->position_x = 0.0f;
        added_clip->position_y = 0.0f;
        IM_CHECK_EQ(added_clip->position_x, 0.0f);
        IM_CHECK_EQ(added_clip->position_y, 0.0f);
        ctx->Yield();

        IM_CHECK_EQ(added_clip->scale_x, 1.0f);
        IM_CHECK_EQ(added_clip->scale_y, 1.0f);

        added_clip->scale_x = 0.01f;
        IM_CHECK_EQ(added_clip->scale_x, 0.01f);

        added_clip->scale_x = 10.0f;
        IM_CHECK_EQ(added_clip->scale_x, 10.0f);

        added_clip->scale_x = 2.5f;
        added_clip->scale_y = 0.5f;
        IM_CHECK_EQ(added_clip->scale_x, 2.5f);
        IM_CHECK_EQ(added_clip->scale_y, 0.5f);

        added_clip->scale_x = 1.0f;
        added_clip->scale_y = 1.0f;
        IM_CHECK_EQ(added_clip->scale_x, 1.0f);
        IM_CHECK_EQ(added_clip->scale_y, 1.0f);
        ctx->Yield();

        IM_CHECK_EQ(added_clip->rotation, 0.0f);

        added_clip->rotation = -360.0f;
        IM_CHECK_EQ(added_clip->rotation, -360.0f);

        added_clip->rotation = 360.0f;
        IM_CHECK_EQ(added_clip->rotation, 360.0f);

        added_clip->rotation = 45.0f;
        IM_CHECK_EQ(added_clip->rotation, 45.0f);

        added_clip->rotation = -90.0f;
        IM_CHECK_EQ(added_clip->rotation, -90.0f);

        added_clip->rotation = 0.0f;
        IM_CHECK_EQ(added_clip->rotation, 0.0f);
        ctx->Yield();

        added_clip->position_x = 100.0f;
        added_clip->position_y = 50.0f;
        added_clip->scale_x = 2.0f;
        added_clip->scale_y = 0.5f;
        added_clip->rotation = 180.0f;

        added_clip->position_x = 0.0f;
        added_clip->position_y = 0.0f;
        added_clip->scale_x = 1.0f;
        added_clip->scale_y = 1.0f;
        added_clip->rotation = 0.0f;

        IM_CHECK_EQ(added_clip->position_x, 0.0f);
        IM_CHECK_EQ(added_clip->position_y, 0.0f);
        IM_CHECK_EQ(added_clip->scale_x, 1.0f);
        IM_CHECK_EQ(added_clip->scale_y, 1.0f);
        IM_CHECK_EQ(added_clip->rotation, 0.0f);
        ctx->Yield();

        IM_CHECK_EQ(added_clip->effects.empty(), true);

        furious::ClipEffect ytpmv_effect;
        ytpmv_effect.effect_id = "auto_ytpmv";
        ytpmv_effect.parameters["sync_period"] = "quarter";
        ytpmv_effect.enabled = true;
        added_clip->effects.push_back(ytpmv_effect);
        IM_CHECK_EQ(added_clip->effects.size(), 1u);
        IM_CHECK_EQ(added_clip->effects[0].enabled, true);
        ctx->Yield();

        double original_duration = added_clip->duration_beats;
        added_clip->duration_beats = 64.0;
        IM_CHECK_EQ(added_clip->duration_beats, 64.0);
        IM_CHECK_GT(added_clip->duration_beats, original_duration);
        ctx->Yield();

        added_clip->effects[0].parameters["sync_period"] = "sixteenth";
        IM_CHECK_EQ(added_clip->effects[0].parameters["sync_period"], "sixteenth");

        added_clip->effects[0].parameters["sync_period"] = "eighth";
        IM_CHECK_EQ(added_clip->effects[0].parameters["sync_period"], "eighth");

        added_clip->effects[0].parameters["sync_period"] = "quarter";
        IM_CHECK_EQ(added_clip->effects[0].parameters["sync_period"], "quarter");

        added_clip->effects[0].parameters["sync_period"] = "half";
        IM_CHECK_EQ(added_clip->effects[0].parameters["sync_period"], "half");

        added_clip->effects[0].parameters["sync_period"] = "measure";
        IM_CHECK_EQ(added_clip->effects[0].parameters["sync_period"], "measure");
        ctx->Yield();

        added_clip->effects[0].parameters["sync_period"] = "quarter";
        double period = 1.0; 
        int expected_loops = static_cast<int>(added_clip->duration_beats / period);
        IM_CHECK_EQ(expected_loops, 64);

        mw.transport_controls().set_playing(true);
        mw.timeline().set_playhead_position(0.0);
        mw.timeline().update(0.1, true);
        mw.transport_controls().set_playing(false);
        ctx->Yield();

        double before_disable_duration = added_clip->duration_beats;
        IM_CHECK_EQ(before_disable_duration, 64.0);

        added_clip->effects[0].enabled = false;
        IM_CHECK_EQ(added_clip->effects[0].enabled, false);
        ctx->Yield();

        std::string clip_id = "test-clip-1";
        IM_CHECK_EQ(mw.timeline_data().clips().size(), 1u);

        mw.timeline_data().remove_clip(clip_id);
        mw.timeline().clear_selection();

        IM_CHECK_EQ(mw.timeline_data().clips().size(), 0u);
        IM_CHECK(mw.timeline_data().find_clip(clip_id) == nullptr);
        IM_CHECK(mw.timeline().selected_clip_id().empty());
        ctx->Yield();

        IM_CHECK_EQ(mw.source_library().sources().size(), 1u);

        mw.timeline_data().remove_clips_by_source(vars.dummy_source_id);
        mw.source_library().remove_source(vars.dummy_source_id);

        IM_CHECK_EQ(mw.source_library().sources().size(), 0u);
        IM_CHECK(mw.source_library().find_source(vars.dummy_source_id) == nullptr);
        ctx->Yield();
    };

    t = IM_REGISTER_TEST(engine, "user_flow", "audio_workflow");
    t->SetVarsDataType<UserFlowTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        vars.main_window->render();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        auto& audio = vars.main_window->audio_engine();

        IM_CHECK_EQ(audio.has_clip(), false);

        audio.set_clip_start_seconds(2.0);
        audio.set_clip_end_seconds(8.0);
        IM_CHECK_EQ(audio.clip_start_seconds(), 2.0);
        IM_CHECK_EQ(audio.clip_end_seconds(), 8.0);
        IM_CHECK_EQ(audio.trimmed_duration_seconds(), 6.0);

        audio.reset_clip_bounds();
        IM_CHECK_EQ(audio.clip_start_seconds(), 0.0);
        IM_CHECK_EQ(audio.clip_end_seconds(), 0.0);

        audio.set_metronome_enabled(true);
        IM_CHECK(audio.metronome_enabled());

        audio.set_bpm(180.0);
        IM_CHECK_EQ(audio.bpm(), 180.0);

        audio.set_beats_per_measure(3);
        IM_CHECK_EQ(audio.beats_per_measure(), 3);
    };

    t = IM_REGISTER_TEST(engine, "user_flow", "track_management");
    t->SetVarsDataType<UserFlowTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        vars.main_window->render();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        auto& td = vars.main_window->timeline_data();

        size_t initial = td.track_count();
        IM_CHECK_GE(initial, 1u);

        for (int i = 0; i < 10; ++i) {
            td.add_track("Track " + std::to_string(i + 2));
        }
        IM_CHECK_EQ(td.track_count(), initial + 10);

        for (int i = 0; i < 5; ++i) {
            td.remove_track(td.track_count() - 1);
        }
        IM_CHECK_EQ(td.track_count(), initial + 5);

        auto dummy = create_dummy_source("track-test-src", "Track Test");
        vars.main_window->source_library().add_source_direct(dummy);

        furious::TimelineClip clip;
        clip.id = "track-test-clip";
        clip.source_id = "track-test-src";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 4.0;
        td.add_clip(clip);

        auto* c = td.find_clip("track-test-clip");
        IM_CHECK(c != nullptr);

        for (size_t track = 0; track < td.track_count(); ++track) {
            c->track_index = track;
            IM_CHECK_EQ(c->track_index, track);

            auto clips = td.clips_on_track(track);
            IM_CHECK_EQ(clips.size(), 1u);
        }

        td.remove_clip("track-test-clip");
        td.remove_clips_by_source("track-test-src");
        vars.main_window->source_library().remove_source("track-test-src");
    };

    t = IM_REGISTER_TEST(engine, "user_flow", "clip_transforms_exhaustive");
    t->SetVarsDataType<UserFlowTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        vars.main_window->render();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        auto& td = vars.main_window->timeline_data();
        auto& sl = vars.main_window->source_library();

        auto src = create_dummy_source("transform-src", "Transform Test");
        sl.add_source_direct(src);

        furious::TimelineClip clip;
        clip.id = "transform-clip";
        clip.source_id = "transform-src";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 4.0;
        td.add_clip(clip);

        auto* c = td.find_clip("transform-clip");
        IM_CHECK(c != nullptr);

        float pos_test_values[] = {-10000.0f, -5000.0f, -100.0f, 0.0f, 100.0f, 5000.0f, 10000.0f};
        for (float val : pos_test_values) {
            c->position_x = val;
            IM_CHECK_EQ(c->position_x, val);
            c->position_y = val;
            IM_CHECK_EQ(c->position_y, val);
        }

        c->position_x = 0.0f;
        c->position_y = 0.0f;
        IM_CHECK_EQ(c->position_x, 0.0f);
        IM_CHECK_EQ(c->position_y, 0.0f);

        float scale_test_values[] = {0.01f, 0.1f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
        for (float val : scale_test_values) {
            c->scale_x = val;
            IM_CHECK_EQ(c->scale_x, val);
            c->scale_y = val;
            IM_CHECK_EQ(c->scale_y, val);
        }

        c->scale_x = 1.0f;
        c->scale_y = 1.0f;
        IM_CHECK_EQ(c->scale_x, 1.0f);
        IM_CHECK_EQ(c->scale_y, 1.0f);

        float rot_test_values[] = {-360.0f, -180.0f, -90.0f, -45.0f, 0.0f, 45.0f, 90.0f, 180.0f, 360.0f};
        for (float val : rot_test_values) {
            c->rotation = val;
            IM_CHECK_EQ(c->rotation, val);
        }

        c->rotation = 0.0f;
        IM_CHECK_EQ(c->rotation, 0.0f);

        c->position_x = 999.0f;
        c->position_y = -888.0f;
        c->scale_x = 3.0f;
        c->scale_y = 0.3f;
        c->rotation = 270.0f;

        c->position_x = 0.0f;
        c->position_y = 0.0f;
        c->scale_x = 1.0f;
        c->scale_y = 1.0f;
        c->rotation = 0.0f;

        IM_CHECK_EQ(c->position_x, 0.0f);
        IM_CHECK_EQ(c->position_y, 0.0f);
        IM_CHECK_EQ(c->scale_x, 1.0f);
        IM_CHECK_EQ(c->scale_y, 1.0f);
        IM_CHECK_EQ(c->rotation, 0.0f);

        td.remove_clip("transform-clip");
        td.remove_clips_by_source("transform-src");
        sl.remove_source("transform-src");
    };

    t = IM_REGISTER_TEST(engine, "user_flow", "ytpmv_effect_workflow");
    t->SetVarsDataType<UserFlowTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        vars.main_window->render();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<UserFlowTestVars>();
        auto& td = vars.main_window->timeline_data();
        auto& sl = vars.main_window->source_library();
        auto& project = vars.main_window->project();

        auto src = create_dummy_source("ytpmv-src", "YTPMV Test");
        sl.add_source_direct(src);

        furious::TimelineClip clip;
        clip.id = "ytpmv-clip";
        clip.source_id = "ytpmv-src";
        clip.track_index = 0;
        clip.start_beat = 0.0;
        clip.duration_beats = 4.0;
        td.add_clip(clip);

        auto* c = td.find_clip("ytpmv-clip");
        IM_CHECK(c != nullptr);

        IM_CHECK_EQ(c->effects.empty(), true);

        furious::ClipEffect effect;
        effect.effect_id = "auto_ytpmv";
        effect.parameters["sync_period"] = "quarter";
        effect.enabled = true;
        c->effects.push_back(effect);
        IM_CHECK_EQ(c->effects[0].enabled, true);

        c->duration_beats = 32.0;
        IM_CHECK_EQ(c->duration_beats, 32.0);

        c->duration_beats = 128.0;
        IM_CHECK_EQ(c->duration_beats, 128.0);

        c->effects[0].parameters["sync_period"] = "sixteenth";
        int loops_16th = static_cast<int>(c->duration_beats / 0.25);
        IM_CHECK_EQ(loops_16th, 512);

        c->effects[0].parameters["sync_period"] = "eighth";
        int loops_8th = static_cast<int>(c->duration_beats / 0.5);
        IM_CHECK_EQ(loops_8th, 256);

        c->effects[0].parameters["sync_period"] = "quarter";
        int loops_quarter = static_cast<int>(c->duration_beats / 1.0);
        IM_CHECK_EQ(loops_quarter, 128);

        c->effects[0].parameters["sync_period"] = "half";
        int loops_half = static_cast<int>(c->duration_beats / 2.0);
        IM_CHECK_EQ(loops_half, 64);

        c->effects[0].parameters["sync_period"] = "measure";
        int loops_measure = static_cast<int>(c->duration_beats / 4.0);
        IM_CHECK_EQ(loops_measure, 32);

        project.tempo().set_bpm(60.0);
        c->effects[0].parameters["sync_period"] = "quarter";
        double period_seconds_60 = project.tempo().beats_to_time(1.0);
        IM_CHECK_EQ(period_seconds_60, 1.0);

        project.tempo().set_bpm(120.0);
        double period_seconds_120 = project.tempo().beats_to_time(1.0);
        IM_CHECK_EQ(period_seconds_120, 0.5);

        double extended_duration = c->duration_beats;
        c->effects[0].enabled = false;
        IM_CHECK_EQ(c->effects[0].enabled, false);

        td.remove_clip("ytpmv-clip");
        IM_CHECK(td.find_clip("ytpmv-clip") == nullptr);

        td.remove_clips_by_source("ytpmv-src");
        sl.remove_source("ytpmv-src");
    };
}
