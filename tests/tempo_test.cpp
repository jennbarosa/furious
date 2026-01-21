#include "furious/core/tempo.hpp"
#include <gtest/gtest.h>

using namespace furious;

class TempoTest : public ::testing::Test {
protected:
    Tempo tempo{120.0};
};

TEST_F(TempoTest, DefaultBpmIs120) {
    Tempo default_tempo;
    EXPECT_DOUBLE_EQ(default_tempo.bpm(), 120.0);
}

TEST_F(TempoTest, BpmCanBeSet) {
    tempo.set_bpm(140.0);
    EXPECT_DOUBLE_EQ(tempo.bpm(), 140.0);
}

TEST_F(TempoTest, BpmClampedToRange) {
    tempo.set_bpm(0.0);
    EXPECT_DOUBLE_EQ(tempo.bpm(), 1.0);
    tempo.set_bpm(1500.0);
    EXPECT_DOUBLE_EQ(tempo.bpm(), 999.0);
}

TEST_F(TempoTest, BeatDuration) {
    EXPECT_DOUBLE_EQ(tempo.beat_duration_seconds(), 0.5);
    tempo.set_bpm(60.0);
    EXPECT_DOUBLE_EQ(tempo.beat_duration_seconds(), 1.0);
}

TEST_F(TempoTest, QuarterNoteSubdivision) {
    double quarter = tempo.subdivision_duration_seconds(NoteSubdivision::Quarter);
    EXPECT_DOUBLE_EQ(quarter, tempo.beat_duration_seconds());
}

TEST_F(TempoTest, EighthNoteSubdivision) {
    double eighth = tempo.subdivision_duration_seconds(NoteSubdivision::Eighth);
    EXPECT_DOUBLE_EQ(eighth, tempo.beat_duration_seconds() / 2.0);
}

TEST_F(TempoTest, SixteenthNoteSubdivision) {
    double sixteenth = tempo.subdivision_duration_seconds(NoteSubdivision::Sixteenth);
    EXPECT_DOUBLE_EQ(sixteenth, tempo.beat_duration_seconds() / 4.0);
}

TEST_F(TempoTest, TimeToBeatsConversion) {
    double beats = tempo.time_to_beats(1.0);
    EXPECT_DOUBLE_EQ(beats, 2.0);
}

TEST_F(TempoTest, BeatsToTimeConversion) {
    double time = tempo.beats_to_time(4.0);
    EXPECT_DOUBLE_EQ(time, 2.0);
}

TEST_F(TempoTest, TimeToSubdivision) {
    EXPECT_EQ(tempo.time_to_subdivision(1.0, NoteSubdivision::Quarter), 2);
    EXPECT_EQ(tempo.time_to_subdivision(1.0, NoteSubdivision::Eighth), 4);
    EXPECT_EQ(tempo.time_to_subdivision(1.0, NoteSubdivision::Sixteenth), 8);
}
