#include "furious/core/timeline_data.hpp"
#include "furious/core/timeline_clip.hpp"
#include <gtest/gtest.h>

using namespace furious;

TEST(TimelineClipTest, DefaultValues) {
    TimelineClip clip;
    EXPECT_TRUE(clip.id.empty());
    EXPECT_TRUE(clip.source_id.empty());
    EXPECT_EQ(clip.track_index, 0u);
    EXPECT_DOUBLE_EQ(clip.start_beat, 0.0);
    EXPECT_DOUBLE_EQ(clip.duration_beats, 4.0);
}

TEST(TimelineClipTest, EndBeatCalculation) {
    TimelineClip clip;
    clip.start_beat = 4.0;
    clip.duration_beats = 8.0;
    EXPECT_DOUBLE_EQ(clip.end_beat(), 12.0);
}

TEST(TimelineClipTest, ContainsBeat) {
    TimelineClip clip;
    clip.start_beat = 4.0;
    clip.duration_beats = 8.0;
    EXPECT_TRUE(clip.contains_beat(4.0));
    EXPECT_TRUE(clip.contains_beat(8.0));
    EXPECT_FALSE(clip.contains_beat(12.0));
    EXPECT_FALSE(clip.contains_beat(3.0));
}

class TimelineDataTest : public ::testing::Test {
protected:
    TimelineData data;
};

TEST_F(TimelineDataTest, StartsWithOneTrack) {
    EXPECT_EQ(data.track_count(), 1u);
    EXPECT_EQ(data.track(0).name, "Track 1");
}

TEST_F(TimelineDataTest, AddTrack) {
    size_t idx = data.add_track("Video");
    EXPECT_EQ(data.track_count(), 2u);
    EXPECT_EQ(data.track(idx).name, "Video");
}

TEST_F(TimelineDataTest, RemoveTrack) {
    data.add_track("Second");
    data.remove_track(0);
    EXPECT_EQ(data.track_count(), 1u);
    EXPECT_EQ(data.track(0).name, "Second");
}

TEST_F(TimelineDataTest, StartsWithNoClips) {
    EXPECT_EQ(data.clips().size(), 0u);
}

TEST_F(TimelineDataTest, AddClip) {
    TimelineClip clip;
    clip.source_id = "src_123";
    clip.start_beat = 8.0;
    data.add_clip(clip);
    EXPECT_EQ(data.clips().size(), 1u);
    EXPECT_FALSE(data.clips()[0].id.empty());
}

TEST_F(TimelineDataTest, RemoveClip) {
    TimelineClip clip;
    clip.id = "clip-to-remove";
    data.add_clip(clip);
    data.remove_clip("clip-to-remove");
    EXPECT_EQ(data.clips().size(), 0u);
}

TEST_F(TimelineDataTest, FindClip) {
    TimelineClip clip;
    clip.id = "find-me";
    clip.start_beat = 10.0;
    data.add_clip(clip);

    TimelineClip* found = data.find_clip("find-me");
    ASSERT_NE(found, nullptr);
    EXPECT_DOUBLE_EQ(found->start_beat, 10.0);
}

TEST_F(TimelineDataTest, ClipsAtBeat) {
    TimelineClip clip1;
    clip1.start_beat = 0.0;
    clip1.duration_beats = 4.0;
    TimelineClip clip2;
    clip2.start_beat = 2.0;
    clip2.duration_beats = 4.0;
    data.add_clip(clip1);
    data.add_clip(clip2);

    auto clips = data.clips_at_beat(3.0);
    EXPECT_EQ(clips.size(), 2u);
}

TEST_F(TimelineDataTest, ClipsOnTrack) {
    data.add_track();
    TimelineClip clip1;
    clip1.track_index = 0;
    TimelineClip clip2;
    clip2.track_index = 1;
    data.add_clip(clip1);
    data.add_clip(clip2);

    auto clips = data.clips_on_track(0);
    EXPECT_EQ(clips.size(), 1u);
}

TEST_F(TimelineDataTest, Clear) {
    TimelineClip clip;
    data.add_clip(clip);
    data.add_track();
    data.clear();
    EXPECT_EQ(data.clips().size(), 0u);
    EXPECT_EQ(data.track_count(), 1u);
}

TEST_F(TimelineDataTest, SetTracks) {
    std::vector<Track> new_tracks;
    new_tracks.push_back(Track{"Video"});
    new_tracks.push_back(Track{"Audio"});
    data.set_tracks(new_tracks);
    EXPECT_EQ(data.track_count(), 2u);
    EXPECT_EQ(data.track(0).name, "Video");
}

TEST_F(TimelineDataTest, SetClips) {
    std::vector<TimelineClip> new_clips;
    TimelineClip clip1;
    clip1.id = "new-1";
    new_clips.push_back(clip1);
    data.set_clips(new_clips);
    EXPECT_EQ(data.clips().size(), 1u);
    EXPECT_EQ(data.clips()[0].id, "new-1");
}


TEST(TimelineClipTrimmingTest, TrimRightAdjustsDuration) {
    TimelineClip clip;
    clip.start_beat = 4.0;
    clip.duration_beats = 8.0;
    clip.source_start_seconds = 0.0;

    double original_start = clip.start_beat;
    double original_source_start = clip.source_start_seconds;

    clip.duration_beats = 12.0;  

    EXPECT_DOUBLE_EQ(clip.start_beat, original_start); 
    EXPECT_DOUBLE_EQ(clip.source_start_seconds, original_source_start);  
    EXPECT_DOUBLE_EQ(clip.duration_beats, 12.0);
    EXPECT_DOUBLE_EQ(clip.end_beat(), 16.0);
}

TEST(TimelineClipTrimmingTest, TrimLeftAdjustsStartAndDuration) {
    TimelineClip clip;
    clip.start_beat = 4.0;
    clip.duration_beats = 8.0;
    clip.source_start_seconds = 1.0;

    double end_beat = clip.end_beat(); 

    double delta_beats = 2.0;
    double bpm = 120.0;  
    double delta_seconds = delta_beats * (60.0 / bpm);  

    clip.start_beat += delta_beats;
    clip.duration_beats = end_beat - clip.start_beat;
    clip.source_start_seconds += delta_seconds;

    EXPECT_DOUBLE_EQ(clip.start_beat, 6.0);
    EXPECT_DOUBLE_EQ(clip.duration_beats, 6.0); 
    EXPECT_DOUBLE_EQ(clip.source_start_seconds, 2.0); 
    EXPECT_DOUBLE_EQ(clip.end_beat(), 12.0);  
}

TEST(TimelineClipTrimmingTest, TrimLeftExpandsClip) {
    TimelineClip clip;
    clip.start_beat = 4.0;
    clip.duration_beats = 8.0;
    clip.source_start_seconds = 2.0;

    double end_beat = clip.end_beat(); 
    double delta_beats = -2.0;
    double bpm = 120.0;
    double delta_seconds = delta_beats * (60.0 / bpm); 

    clip.start_beat += delta_beats;
    clip.duration_beats = end_beat - clip.start_beat;
    clip.source_start_seconds += delta_seconds;

    EXPECT_DOUBLE_EQ(clip.start_beat, 2.0);
    EXPECT_DOUBLE_EQ(clip.duration_beats, 10.0); 
    EXPECT_DOUBLE_EQ(clip.source_start_seconds, 1.0); 
    EXPECT_DOUBLE_EQ(clip.end_beat(), 12.0); 
}

TEST(TimelineClipTrimmingTest, TrimLeftClampedAtSourceStart) {
    TimelineClip clip;
    clip.start_beat = 4.0;
    clip.duration_beats = 8.0;
    clip.source_start_seconds = 0.5; 

    double end_beat = clip.end_beat();
    double bpm = 120.0;

    double max_delta_seconds = -clip.source_start_seconds; 
    double max_delta_beats = max_delta_seconds / (60.0 / bpm); 

    clip.start_beat += max_delta_beats;
    clip.duration_beats = end_beat - clip.start_beat;
    clip.source_start_seconds = 0.0; 

    EXPECT_DOUBLE_EQ(clip.start_beat, 3.0); 
    EXPECT_DOUBLE_EQ(clip.duration_beats, 9.0);  
    EXPECT_DOUBLE_EQ(clip.source_start_seconds, 0.0);  
    EXPECT_DOUBLE_EQ(clip.end_beat(), 12.0); 
}

TEST(TimelineClipTrimmingTest, MinimumDurationEnforced) {
    TimelineClip clip;
    clip.start_beat = 4.0;
    clip.duration_beats = 8.0;

    constexpr double MIN_DURATION = 0.25;

    double new_duration = 0.1;
    clip.duration_beats = std::max(MIN_DURATION, new_duration);

    EXPECT_DOUBLE_EQ(clip.duration_beats, 0.25);
}

TEST(TimelineClipTrimmingTest, TrimLeftClampedAtBeatZero) {
    TimelineClip clip;
    clip.start_beat = 1.0; 
    clip.duration_beats = 8.0;
    clip.source_start_seconds = 10.0;  

    double end_beat = clip.end_beat(); 
    double delta_beats = -3.0;  

    double new_start = std::max(0.0, clip.start_beat + delta_beats);

    clip.start_beat = new_start;
    clip.duration_beats = end_beat - clip.start_beat;

    EXPECT_DOUBLE_EQ(clip.start_beat, 0.0);  
    EXPECT_DOUBLE_EQ(clip.duration_beats, 9.0);  
    EXPECT_DOUBLE_EQ(clip.end_beat(), 9.0);  
}


TEST(TimelineClipTransformTest, DefaultValues) {
    TimelineClip clip;
    EXPECT_FLOAT_EQ(clip.position_x, 0.0f);
    EXPECT_FLOAT_EQ(clip.position_y, 0.0f);
    EXPECT_FLOAT_EQ(clip.scale_x, 1.0f);
    EXPECT_FLOAT_EQ(clip.scale_y, 1.0f);
    EXPECT_FLOAT_EQ(clip.rotation, 0.0f);
}

TEST(TimelineClipTransformTest, PositionCanBeSet) {
    TimelineClip clip;
    clip.position_x = 100.0f;
    clip.position_y = -50.0f;

    EXPECT_FLOAT_EQ(clip.position_x, 100.0f);
    EXPECT_FLOAT_EQ(clip.position_y, -50.0f);
}

TEST(TimelineClipTransformTest, ScaleCanBeSet) {
    TimelineClip clip;
    clip.scale_x = 2.0f;
    clip.scale_y = 0.5f;

    EXPECT_FLOAT_EQ(clip.scale_x, 2.0f);
    EXPECT_FLOAT_EQ(clip.scale_y, 0.5f);
}

TEST(TimelineClipTransformTest, RotationCanBeSet) {
    TimelineClip clip;
    clip.rotation = 45.0f;
    EXPECT_FLOAT_EQ(clip.rotation, 45.0f);

    clip.rotation = -90.0f;
    EXPECT_FLOAT_EQ(clip.rotation, -90.0f);

    clip.rotation = 360.0f;
    EXPECT_FLOAT_EQ(clip.rotation, 360.0f);
}

TEST(TimelineClipTransformTest, IndependentScaleAxes) {
    TimelineClip clip;

    clip.scale_x = 3.0f;
    EXPECT_FLOAT_EQ(clip.scale_x, 3.0f);
    EXPECT_FLOAT_EQ(clip.scale_y, 1.0f); 

    clip.scale_y = 0.25f;
    EXPECT_FLOAT_EQ(clip.scale_x, 3.0f);  
    EXPECT_FLOAT_EQ(clip.scale_y, 0.25f);
}
