#include "furious/core/project_data.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

using namespace furious;

class ProjectDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = std::filesystem::temp_directory_path() / "test_project.furious";
    }

    void TearDown() override {
        if (std::filesystem::exists(test_file_)) {
            std::filesystem::remove(test_file_);
        }
    }

    std::filesystem::path test_file_;
};

TEST_F(ProjectDataTest, DefaultValues) {
    ProjectData data;
    EXPECT_EQ(data.name, "Untitled Project");
    EXPECT_DOUBLE_EQ(data.bpm, 120.0);
    EXPECT_EQ(data.grid_subdivision, NoteSubdivision::Quarter);
    EXPECT_DOUBLE_EQ(data.fps, 30.0);
    EXPECT_FALSE(data.metronome_enabled);
    EXPECT_TRUE(data.follow_playhead);
    EXPECT_FALSE(data.loop_enabled);
}

TEST_F(ProjectDataTest, SaveAndLoadRoundTrip) {
    ProjectData original;
    original.name = "Test Project";
    original.bpm = 140.0;
    original.grid_subdivision = NoteSubdivision::Eighth;
    original.fps = 60.0;
    original.metronome_enabled = true;
    original.audio_filepath = "/path/to/audio.wav";

    ASSERT_TRUE(original.save_to_file(test_file_.string()));

    ProjectData loaded;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), loaded));

    EXPECT_EQ(loaded.name, original.name);
    EXPECT_DOUBLE_EQ(loaded.bpm, original.bpm);
    EXPECT_EQ(loaded.grid_subdivision, original.grid_subdivision);
    EXPECT_DOUBLE_EQ(loaded.fps, original.fps);
    EXPECT_EQ(loaded.metronome_enabled, original.metronome_enabled);
    EXPECT_EQ(loaded.audio_filepath, original.audio_filepath);
}

TEST_F(ProjectDataTest, LoadFailsForNonexistentFile) {
    ProjectData data;
    EXPECT_FALSE(ProjectData::load_from_file("/nonexistent/path/project.furious", data));
}

TEST_F(ProjectDataTest, LoadFailsForInvalidJson) {
    std::ofstream file(test_file_);
    file << "not valid json {{{";
    file.close();

    ProjectData data;
    EXPECT_FALSE(ProjectData::load_from_file(test_file_.string(), data));
}

TEST_F(ProjectDataTest, LoadHandlesMissingOptionalFields) {
    std::ofstream file(test_file_);
    file << R"({"version": 1})";
    file.close();

    ProjectData data;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), data));
    EXPECT_EQ(data.name, "Untitled Project");
    EXPECT_DOUBLE_EQ(data.bpm, 120.0);
}

TEST_F(ProjectDataTest, GridSubdivisionRoundTrip) {
    for (auto subdiv : {NoteSubdivision::Quarter, NoteSubdivision::Eighth, NoteSubdivision::Sixteenth}) {
        ProjectData original;
        original.grid_subdivision = subdiv;
        ASSERT_TRUE(original.save_to_file(test_file_.string()));

        ProjectData loaded;
        ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), loaded));
        EXPECT_EQ(loaded.grid_subdivision, subdiv);
    }
}

TEST_F(ProjectDataTest, ClipTransformPropertiesRoundTrip) {
    ProjectData original;

    TimelineClip clip;
    clip.id = "transform-test";
    clip.source_id = "source-1";
    clip.position_x = 150.0f;
    clip.position_y = -75.5f;
    clip.scale_x = 2.5f;
    clip.scale_y = 0.75f;
    clip.rotation = 45.0f;
    original.clips.push_back(clip);

    ASSERT_TRUE(original.save_to_file(test_file_.string()));

    ProjectData loaded;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), loaded));

    ASSERT_EQ(loaded.clips.size(), 1u);
    const auto& loaded_clip = loaded.clips[0];
    EXPECT_FLOAT_EQ(loaded_clip.position_x, 150.0f);
    EXPECT_FLOAT_EQ(loaded_clip.position_y, -75.5f);
    EXPECT_FLOAT_EQ(loaded_clip.scale_x, 2.5f);
    EXPECT_FLOAT_EQ(loaded_clip.scale_y, 0.75f);
    EXPECT_FLOAT_EQ(loaded_clip.rotation, 45.0f);
}

TEST_F(ProjectDataTest, ClipTransformDefaultsOnLoad) {
    std::ofstream file(test_file_);
    file << R"({
        "version": 1,
        "clips": [
            {"id": "old-clip", "source_id": "src-1"}
        ]
    })";
    file.close();

    ProjectData data;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), data));

    ASSERT_EQ(data.clips.size(), 1u);
    const auto& clip = data.clips[0];
    EXPECT_FLOAT_EQ(clip.position_x, 0.0f);
    EXPECT_FLOAT_EQ(clip.position_y, 0.0f);
    EXPECT_FLOAT_EQ(clip.scale_x, 1.0f);
    EXPECT_FLOAT_EQ(clip.scale_y, 1.0f);
    EXPECT_FLOAT_EQ(clip.rotation, 0.0f);
}

TEST_F(ProjectDataTest, BackwardsCompatibilityWithViewportProperties) {
    std::ofstream file(test_file_);
    file << R"({
        "version": 1,
        "clips": [
            {
                "id": "old-format-clip",
                "source_id": "src-1",
                "viewport_x": 100.0,
                "viewport_y": 200.0,
                "viewport_scale": 1.5
            }
        ]
    })";
    file.close();

    ProjectData data;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), data));

    ASSERT_EQ(data.clips.size(), 1u);
    const auto& clip = data.clips[0];
    EXPECT_FLOAT_EQ(clip.position_x, 100.0f);
    EXPECT_FLOAT_EQ(clip.position_y, 200.0f);
    EXPECT_FLOAT_EQ(clip.scale_x, 1.5f);
    EXPECT_FLOAT_EQ(clip.scale_y, 1.5f);  
    EXPECT_FLOAT_EQ(clip.rotation, 0.0f);  
}
