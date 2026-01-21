#include "furious/ui/timeline.hpp"
#include "furious/core/project.hpp"
#include <gtest/gtest.h>

using namespace furious;

class TimelineTest : public ::testing::Test {
protected:
    Project project;
    Timeline timeline{project};
};

TEST_F(TimelineTest, PlayheadStartsAtZero) {
    EXPECT_DOUBLE_EQ(timeline.playhead_position(), 0.0);
}

TEST_F(TimelineTest, PlayheadDoesNotMoveWhenNotPlaying) {
    timeline.update(1.0, false);
    EXPECT_DOUBLE_EQ(timeline.playhead_position(), 0.0);
}

TEST_F(TimelineTest, PlayheadMovesWhenPlaying) {
    timeline.update(1.0, true);
    EXPECT_GT(timeline.playhead_position(), 0.0);
}

TEST_F(TimelineTest, PlayheadMovesAtCorrectRate) {
    project.tempo().set_bpm(120.0);
    timeline.update(1.0, true);
    EXPECT_DOUBLE_EQ(timeline.playhead_position(), 2.0);
}

TEST_F(TimelineTest, PlayheadCanBeSetManually) {
    timeline.set_playhead_position(10.0);
    EXPECT_DOUBLE_EQ(timeline.playhead_position(), 10.0);
}

TEST_F(TimelineTest, ZoomDefaultsToOne) {
    EXPECT_FLOAT_EQ(timeline.zoom(), 1.0f);
}

TEST_F(TimelineTest, ZoomCanBeSet) {
    timeline.set_zoom(2.0f);
    EXPECT_FLOAT_EQ(timeline.zoom(), 2.0f);
}

TEST_F(TimelineTest, ZoomClampedToRange) {
    timeline.set_zoom(0.01f);
    EXPECT_FLOAT_EQ(timeline.zoom(), 0.1f);
    timeline.set_zoom(100.0f);
    EXPECT_FLOAT_EQ(timeline.zoom(), 10.0f);
}

TEST_F(TimelineTest, ScrollOffsetDefaultsToZero) {
    EXPECT_FLOAT_EQ(timeline.scroll_offset(), 0.0f);
}

TEST_F(TimelineTest, ScrollOffsetCanBeSet) {
    timeline.set_scroll_offset(100.0f);
    EXPECT_FLOAT_EQ(timeline.scroll_offset(), 100.0f);
}

TEST_F(TimelineTest, ScrollOffsetClampedToZero) {
    timeline.set_scroll_offset(-50.0f);
    EXPECT_FLOAT_EQ(timeline.scroll_offset(), 0.0f);
}

TEST_F(TimelineTest, ScreenXToBeats) {
    float canvas_x = 0.0f;
    float screen_x = 100.0f;
    double beats = timeline.screen_x_to_beats(screen_x, canvas_x);
    EXPECT_DOUBLE_EQ(beats, 1.0);
}

TEST_F(TimelineTest, ScreenXToBeatsWithZoom) {
    timeline.set_zoom(2.0f);
    float canvas_x = 0.0f;
    float screen_x = 200.0f;
    double beats = timeline.screen_x_to_beats(screen_x, canvas_x);
    EXPECT_DOUBLE_EQ(beats, 1.0);
}

TEST_F(TimelineTest, ClipDurationDefaultsToZero) {
    EXPECT_DOUBLE_EQ(timeline.clip_duration_beats(), 0.0);
}

TEST_F(TimelineTest, ClipDurationCanBeSet) {
    timeline.set_clip_duration_beats(16.0);
    EXPECT_DOUBLE_EQ(timeline.clip_duration_beats(), 16.0);
}

TEST_F(TimelineTest, FollowPlayheadEnabledByDefault) {
    EXPECT_TRUE(timeline.follow_playhead());
}

TEST_F(TimelineTest, FollowPlayheadCanBeToggled) {
    timeline.set_follow_playhead(false);
    EXPECT_FALSE(timeline.follow_playhead());
    timeline.set_follow_playhead(true);
    EXPECT_TRUE(timeline.follow_playhead());
}

TEST_F(TimelineTest, ZoomYDefaultsToOne) {
    EXPECT_FLOAT_EQ(timeline.zoom_y(), 1.0f);
}

TEST_F(TimelineTest, ZoomYClampedToRange) {
    timeline.set_zoom_y(0.1f);
    EXPECT_FLOAT_EQ(timeline.zoom_y(), 0.5f);
    timeline.set_zoom_y(10.0f);
    EXPECT_FLOAT_EQ(timeline.zoom_y(), 3.0f);
}

TEST_F(TimelineTest, ScrollOffsetYDefaultsToZero) {
    EXPECT_FLOAT_EQ(timeline.scroll_offset_y(), 0.0f);
}

TEST_F(TimelineTest, ScrollOffsetYCanBeSet) {
    timeline.set_scroll_offset_y(100.0f);
    EXPECT_FLOAT_EQ(timeline.scroll_offset_y(), 100.0f);
}

TEST_F(TimelineTest, FpsDefaultsTo30) {
    EXPECT_DOUBLE_EQ(timeline.fps(), 30.0);
}

TEST_F(TimelineTest, FpsCanBeSet) {
    timeline.set_fps(60.0);
    EXPECT_DOUBLE_EQ(timeline.fps(), 60.0);
}
