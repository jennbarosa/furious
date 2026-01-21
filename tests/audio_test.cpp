#include <gtest/gtest.h>
#include "furious/audio/audio_clip.hpp"
#include "furious/audio/audio_engine.hpp"

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

} // namespace
} // namespace furious
