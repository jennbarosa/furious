#include <gtest/gtest.h>
#include "furious/audio/audio_clip.hpp"
#include "furious/audio/audio_engine.hpp"
#include "furious/audio/audio_buffer.hpp"
#include <memory>

namespace furious {
namespace {

class AudioClipTest : public ::testing::Test {
protected:
    AudioClip clip;
};

TEST_F(AudioClipTest, DefaultStateNotLoaded) {
    EXPECT_FALSE(clip.is_loaded());
    EXPECT_TRUE(clip.filepath().empty());
    EXPECT_EQ(clip.sample_rate(), 0u);
    EXPECT_EQ(clip.channels(), 0u);
    EXPECT_EQ(clip.total_frames(), 0u);
    EXPECT_DOUBLE_EQ(clip.duration_seconds(), 0.0);
}

TEST_F(AudioClipTest, LoadNonexistentFileFails) {
    EXPECT_FALSE(clip.load("/nonexistent/path/to/audio.wav"));
    EXPECT_FALSE(clip.is_loaded());
}

class AudioEngineTest : public ::testing::Test {
protected:
    AudioEngine engine;
};

TEST_F(AudioEngineTest, DefaultState) {
    EXPECT_FALSE(engine.is_playing());
    EXPECT_FALSE(engine.has_clip());
    EXPECT_DOUBLE_EQ(engine.playhead_seconds(), 0.0);
}

TEST_F(AudioEngineTest, InitializeAndShutdown) {
    EXPECT_TRUE(engine.initialize());
    engine.shutdown();
}

TEST_F(AudioEngineTest, PlayPauseStop) {
    engine.play();
    EXPECT_TRUE(engine.is_playing());
    engine.pause();
    EXPECT_FALSE(engine.is_playing());
    engine.play();
    engine.stop();
    EXPECT_FALSE(engine.is_playing());
    EXPECT_DOUBLE_EQ(engine.playhead_seconds(), 0.0);
}

TEST_F(AudioEngineTest, MetronomeDefaultsToDisabled) {
    EXPECT_FALSE(engine.metronome_enabled());
}

TEST_F(AudioEngineTest, MetronomeCanBeToggled) {
    engine.set_metronome_enabled(true);
    EXPECT_TRUE(engine.metronome_enabled());
    engine.set_metronome_enabled(false);
    EXPECT_FALSE(engine.metronome_enabled());
}

TEST_F(AudioEngineTest, ClickSoundsGenerated) {
    EXPECT_FALSE(engine.click_sound_high().empty());
    EXPECT_FALSE(engine.click_sound_low().empty());
}

TEST_F(AudioEngineTest, BpmDefaultsTo120) {
    EXPECT_DOUBLE_EQ(engine.bpm(), 120.0);
}

TEST_F(AudioEngineTest, BpmCanBeSet) {
    engine.set_bpm(140.0);
    EXPECT_DOUBLE_EQ(engine.bpm(), 140.0);
}

TEST_F(AudioEngineTest, BeatsPerMeasureDefaultsTo4) {
    EXPECT_EQ(engine.beats_per_measure(), 4);
}

TEST_F(AudioEngineTest, BeatsPerMeasureCanBeSet) {
    engine.set_beats_per_measure(3);
    EXPECT_EQ(engine.beats_per_measure(), 3);
}

TEST_F(AudioEngineTest, ClipBoundsCanBeSet) {
    engine.set_clip_start_seconds(5.0);
    engine.set_clip_end_seconds(10.0);
    EXPECT_DOUBLE_EQ(engine.clip_start_seconds(), 5.0);
    EXPECT_DOUBLE_EQ(engine.clip_end_seconds(), 10.0);
}

TEST_F(AudioEngineTest, TrimmedDuration) {
    engine.set_clip_start_seconds(2.0);
    engine.set_clip_end_seconds(7.0);
    EXPECT_DOUBLE_EQ(engine.trimmed_duration_seconds(), 5.0);
}

TEST_F(AudioEngineTest, PlayheadCanBeSet) {
    engine.set_playhead_seconds(5.0);
    EXPECT_DOUBLE_EQ(engine.playhead_seconds(), 5.0);
}

TEST_F(AudioEngineTest, AdvancePlayhead) {
    engine.play();
    uint64_t initial = engine.playhead_frame();
    engine.advance_playhead(1024);
    EXPECT_EQ(engine.playhead_frame(), initial + 1024);
}

TEST_F(AudioEngineTest, ActiveClipsInitiallyEmpty) {
    EXPECT_TRUE(engine.active_clips().empty());
}

TEST_F(AudioEngineTest, SetActiveClips) {
    std::vector<float> samples = {0.1f, 0.2f, 0.3f, 0.4f};
    auto buffer = std::make_shared<const AudioBuffer>(std::move(samples), 44100, 2);

    ClipAudioState state;
    state.buffer = buffer;
    state.timeline_start_frame = 0;
    state.duration_frames = 2;
    state.volume = 0.8f;

    engine.set_active_clips({state});
    engine.swap_active_clips_if_pending();

    EXPECT_EQ(engine.active_clips().size(), 1u);
}

TEST_F(AudioEngineTest, SetActiveClipsMultiple) {
    std::vector<float> samples1 = {0.1f, 0.2f};
    std::vector<float> samples2 = {0.3f, 0.4f};
    auto buffer1 = std::make_shared<const AudioBuffer>(std::move(samples1), 44100, 2);
    auto buffer2 = std::make_shared<const AudioBuffer>(std::move(samples2), 44100, 2);

    ClipAudioState state1;
    state1.buffer = buffer1;
    state1.timeline_start_frame = 0;
    state1.duration_frames = 1;

    ClipAudioState state2;
    state2.buffer = buffer2;
    state2.timeline_start_frame = 44100;
    state2.duration_frames = 1;

    engine.set_active_clips({state1, state2});
    engine.swap_active_clips_if_pending();

    EXPECT_EQ(engine.active_clips().size(), 2u);
}

TEST_F(AudioEngineTest, SetActiveClipsReplacePrevious) {
    std::vector<float> samples = {0.1f, 0.2f};
    auto buffer = std::make_shared<const AudioBuffer>(std::move(samples), 44100, 2);

    ClipAudioState state;
    state.buffer = buffer;

    engine.set_active_clips({state, state, state});
    engine.swap_active_clips_if_pending();
    EXPECT_EQ(engine.active_clips().size(), 3u);

    engine.set_active_clips({state});
    engine.swap_active_clips_if_pending();
    EXPECT_EQ(engine.active_clips().size(), 1u);
}

TEST_F(AudioEngineTest, ClearActiveClips) {
    std::vector<float> samples = {0.1f, 0.2f};
    auto buffer = std::make_shared<const AudioBuffer>(std::move(samples), 44100, 2);

    ClipAudioState state;
    state.buffer = buffer;

    engine.set_active_clips({state});
    engine.swap_active_clips_if_pending();
    EXPECT_EQ(engine.active_clips().size(), 1u);

    engine.set_active_clips({});
    engine.swap_active_clips_if_pending();
    EXPECT_TRUE(engine.active_clips().empty());
}

class ClipAudioStateTest : public ::testing::Test {};

TEST_F(ClipAudioStateTest, DefaultValues) {
    ClipAudioState state;
    EXPECT_EQ(state.buffer, nullptr);
    EXPECT_EQ(state.timeline_start_frame, 0);
    EXPECT_EQ(state.source_offset_frames, 0);
    EXPECT_EQ(state.duration_frames, 0);
    EXPECT_FLOAT_EQ(state.volume, 1.0f);
    EXPECT_FALSE(state.use_looped_audio);
    EXPECT_EQ(state.loop_start_frames, 0);
    EXPECT_EQ(state.loop_duration_frames, 0);
}

TEST_F(ClipAudioStateTest, WithBuffer) {
    std::vector<float> samples = {0.1f, 0.2f, 0.3f, 0.4f};
    auto buffer = std::make_shared<const AudioBuffer>(std::move(samples), 44100, 2);

    ClipAudioState state;
    state.buffer = buffer;
    state.timeline_start_frame = 1000;
    state.source_offset_frames = 500;
    state.duration_frames = 2000;
    state.volume = 0.5f;

    EXPECT_NE(state.buffer, nullptr);
    EXPECT_EQ(state.buffer->frame_count(), 2u);
    EXPECT_EQ(state.timeline_start_frame, 1000);
    EXPECT_EQ(state.source_offset_frames, 500);
    EXPECT_EQ(state.duration_frames, 2000);
    EXPECT_FLOAT_EQ(state.volume, 0.5f);
}

TEST_F(ClipAudioStateTest, LoopedAudioSettings) {
    ClipAudioState state;
    state.use_looped_audio = true;
    state.loop_start_frames = 44100;
    state.loop_duration_frames = 22050;

    EXPECT_TRUE(state.use_looped_audio);
    EXPECT_EQ(state.loop_start_frames, 44100);
    EXPECT_EQ(state.loop_duration_frames, 22050);
}

TEST_F(AudioEngineTest, SampleRateDefault) {
    EXPECT_EQ(engine.sample_rate(), 44100u);
}

TEST_F(AudioEngineTest, ResetClipBounds) {
    engine.set_clip_start_seconds(5.0);
    engine.set_clip_end_seconds(10.0);
    engine.reset_clip_bounds();
    EXPECT_DOUBLE_EQ(engine.clip_start_seconds(), 0.0);
    EXPECT_DOUBLE_EQ(engine.clip_end_seconds(), 0.0);
}

TEST_F(AudioEngineTest, ClipStartFrame) {
    engine.set_clip_start_seconds(1.0);
    EXPECT_EQ(engine.clip_start_frame(), 44100u);
}

TEST_F(AudioEngineTest, ClipEndFrame) {
    engine.set_clip_end_seconds(2.0);
    EXPECT_EQ(engine.clip_end_frame(), 88200u);
}

} // namespace
} // namespace furious
