#include "imgui.h"
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "furious/audio/audio_engine.hpp"
#include "furious/audio/audio_buffer.hpp"
#include "furious/core/project.hpp"
#include <cmath>
#include <memory>

namespace {

struct AudioEngineTestVars {
    furious::AudioEngine engine;
    furious::Project project;

    std::shared_ptr<const furious::AudioBuffer> create_test_buffer(
        size_t frame_count,
        uint32_t sample_rate = 44100,
        uint32_t channels = 2
    ) {
        std::vector<float> samples(frame_count * channels);
        for (size_t i = 0; i < samples.size(); ++i) {
            float t = static_cast<float>(i / channels) / static_cast<float>(sample_rate);
            samples[i] = std::sin(2.0f * 3.14159f * 440.0f * t) * 0.5f;
        }
        return std::make_shared<const furious::AudioBuffer>(std::move(samples), sample_rate, channels);
    }
};

}

void RegisterAudioEngineTests(ImGuiTestEngine* engine) {
    ImGuiTest* t = nullptr;

    t = IM_REGISTER_TEST(engine, "audio_engine", "initialize_shutdown");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        IM_CHECK_EQ(vars.engine.initialize(), true);
        IM_CHECK_EQ(vars.engine.sample_rate(), 44100u);
        vars.engine.shutdown();
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "play_pause_stop");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        IM_CHECK_EQ(vars.engine.is_playing(), false);

        vars.engine.play();
        IM_CHECK_EQ(vars.engine.is_playing(), true);

        vars.engine.pause();
        IM_CHECK_EQ(vars.engine.is_playing(), false);

        vars.engine.play();
        vars.engine.stop();
        IM_CHECK_EQ(vars.engine.is_playing(), false);
        IM_CHECK(std::fabs(vars.engine.playhead_seconds()) < 0.001);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "playhead_positioning");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        vars.engine.set_playhead_seconds(0.0);
        IM_CHECK(std::fabs(vars.engine.playhead_seconds()) < 0.001);

        vars.engine.set_playhead_seconds(5.0);
        IM_CHECK(std::fabs(vars.engine.playhead_seconds() - 5.0) < 0.001);

        vars.engine.set_playhead_seconds(10.5);
        IM_CHECK(std::fabs(vars.engine.playhead_seconds() - 10.5) < 0.001);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "playhead_frame_advance");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        vars.engine.set_playhead_seconds(0.0);
        vars.engine.play();

        uint64_t initial_frame = vars.engine.playhead_frame();
        vars.engine.advance_playhead(1024);

        IM_CHECK_EQ(vars.engine.playhead_frame(), initial_frame + 1024);

        vars.engine.advance_playhead(2048);
        IM_CHECK_EQ(vars.engine.playhead_frame(), initial_frame + 1024 + 2048);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "bpm_settings");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        IM_CHECK(std::fabs(vars.engine.bpm() - 120.0) < 0.001);

        vars.engine.set_bpm(140.0);
        IM_CHECK(std::fabs(vars.engine.bpm() - 140.0) < 0.001);

        vars.engine.set_bpm(60.0);
        IM_CHECK(std::fabs(vars.engine.bpm() - 60.0) < 0.001);

        vars.engine.set_bpm(200.0);
        IM_CHECK(std::fabs(vars.engine.bpm() - 200.0) < 0.001);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "metronome_toggle");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        IM_CHECK_EQ(vars.engine.metronome_enabled(), false);

        vars.engine.set_metronome_enabled(true);
        IM_CHECK_EQ(vars.engine.metronome_enabled(), true);

        vars.engine.set_metronome_enabled(false);
        IM_CHECK_EQ(vars.engine.metronome_enabled(), false);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "click_sounds_exist");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        IM_CHECK(!vars.engine.click_sound_high().empty());
        IM_CHECK(!vars.engine.click_sound_low().empty());
        IM_CHECK_GT(vars.engine.click_sound_high().size(), 100u);
        IM_CHECK_GT(vars.engine.click_sound_low().size(), 100u);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "clip_trimming");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        vars.engine.set_clip_start_seconds(0.0);
        vars.engine.set_clip_end_seconds(0.0);

        IM_CHECK(std::fabs(vars.engine.clip_start_seconds()) < 0.001);
        IM_CHECK(std::fabs(vars.engine.clip_end_seconds()) < 0.001);

        vars.engine.set_clip_start_seconds(2.0);
        vars.engine.set_clip_end_seconds(8.0);

        IM_CHECK(std::fabs(vars.engine.clip_start_seconds() - 2.0) < 0.001);
        IM_CHECK(std::fabs(vars.engine.clip_end_seconds() - 8.0) < 0.001);
        IM_CHECK(std::fabs(vars.engine.trimmed_duration_seconds() - 6.0) < 0.001);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "clip_frame_bounds");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        vars.engine.set_clip_start_seconds(1.0);
        vars.engine.set_clip_end_seconds(2.0);

        IM_CHECK_EQ(vars.engine.clip_start_frame(), 44100u);
        IM_CHECK_EQ(vars.engine.clip_end_frame(), 88200u);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "reset_clip_bounds");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        vars.engine.set_clip_start_seconds(5.0);
        vars.engine.set_clip_end_seconds(15.0);

        vars.engine.reset_clip_bounds();

        IM_CHECK(std::fabs(vars.engine.clip_start_seconds()) < 0.001);
        IM_CHECK(std::fabs(vars.engine.clip_end_seconds()) < 0.001);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "active_clips_management");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        IM_CHECK(vars.engine.active_clips().empty());

        auto buffer = vars.create_test_buffer(44100);

        furious::ClipAudioState state;
        state.buffer = buffer;
        state.timeline_start_frame = 0;
        state.duration_frames = 44100;
        state.volume = 1.0f;

        vars.engine.set_active_clips({state});
        vars.engine.swap_active_clips_if_pending();

        IM_CHECK_EQ(vars.engine.active_clips().size(), 1u);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "multiple_active_clips");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        auto buffer1 = vars.create_test_buffer(44100);
        auto buffer2 = vars.create_test_buffer(88200);
        auto buffer3 = vars.create_test_buffer(22050);

        furious::ClipAudioState state1, state2, state3;
        state1.buffer = buffer1;
        state1.timeline_start_frame = 0;
        state1.duration_frames = 44100;

        state2.buffer = buffer2;
        state2.timeline_start_frame = 44100;
        state2.duration_frames = 88200;

        state3.buffer = buffer3;
        state3.timeline_start_frame = 132300;
        state3.duration_frames = 22050;

        vars.engine.set_active_clips({state1, state2, state3});
        vars.engine.swap_active_clips_if_pending();

        IM_CHECK_EQ(vars.engine.active_clips().size(), 3u);

        const auto& clips = vars.engine.active_clips();
        IM_CHECK_EQ(clips[0].timeline_start_frame, 0);
        IM_CHECK_EQ(clips[1].timeline_start_frame, 44100);
        IM_CHECK_EQ(clips[2].timeline_start_frame, 132300);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "clip_audio_state_properties");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        auto buffer = vars.create_test_buffer(44100);

        furious::ClipAudioState state;
        state.buffer = buffer;
        state.timeline_start_frame = 1000;
        state.source_offset_frames = 500;
        state.duration_frames = 2000;
        state.volume = 0.75f;
        state.use_looped_audio = true;
        state.loop_start_frames = 100;
        state.loop_duration_frames = 400;

        vars.engine.set_active_clips({state});
        vars.engine.swap_active_clips_if_pending();

        const auto& clip = vars.engine.active_clips()[0];
        IM_CHECK_EQ(clip.timeline_start_frame, 1000);
        IM_CHECK_EQ(clip.source_offset_frames, 500);
        IM_CHECK_EQ(clip.duration_frames, 2000);
        IM_CHECK(std::fabs(clip.volume - 0.75f) < 0.001f);
        IM_CHECK_EQ(clip.use_looped_audio, true);
        IM_CHECK_EQ(clip.loop_start_frames, 100);
        IM_CHECK_EQ(clip.loop_duration_frames, 400);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "clear_active_clips");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        auto buffer = vars.create_test_buffer(44100);

        furious::ClipAudioState state;
        state.buffer = buffer;

        vars.engine.set_active_clips({state, state, state});
        vars.engine.swap_active_clips_if_pending();
        IM_CHECK_EQ(vars.engine.active_clips().size(), 3u);

        vars.engine.set_active_clips({});
        vars.engine.swap_active_clips_if_pending();
        IM_CHECK(vars.engine.active_clips().empty());
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "beats_per_measure");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        IM_CHECK_EQ(vars.engine.beats_per_measure(), 4);

        vars.engine.set_beats_per_measure(3);
        IM_CHECK_EQ(vars.engine.beats_per_measure(), 3);

        vars.engine.set_beats_per_measure(6);
        IM_CHECK_EQ(vars.engine.beats_per_measure(), 6);

        vars.engine.set_beats_per_measure(4);
        IM_CHECK_EQ(vars.engine.beats_per_measure(), 4);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "bpm_sync_with_project");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        vars.project.tempo().set_bpm(150.0);
        vars.engine.set_bpm(vars.project.tempo().bpm());

        IM_CHECK(std::fabs(vars.engine.bpm() - 150.0) < 0.001);

        double samples_per_second = static_cast<double>(vars.engine.sample_rate());
        double beats_per_second = 150.0 / 60.0;
        double samples_per_beat = samples_per_second / beats_per_second;

        IM_CHECK(std::fabs(samples_per_beat - 17640.0) < 1.0);
    };

    t = IM_REGISTER_TEST(engine, "audio_engine", "playhead_time_accuracy");
    t->SetVarsDataType<AudioEngineTestVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx) {
        ImGui::Begin("Audio Engine Test");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx) {
        auto& vars = ctx->GetVars<AudioEngineTestVars>();

        vars.engine.set_playhead_seconds(1.0);

        IM_CHECK_EQ(vars.engine.playhead_frame(), 44100u);

        vars.engine.set_playhead_seconds(2.5);
        IM_CHECK_EQ(vars.engine.playhead_frame(), 110250u);

        double seconds = vars.engine.playhead_seconds();
        IM_CHECK(std::fabs(seconds - 2.5) < 0.0001);
    };
}
