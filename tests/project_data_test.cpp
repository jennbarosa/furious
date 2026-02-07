#include "furious/core/project_data.hpp"
#include "furious/core/pattern_library.hpp"
#include "furious/core/pattern_commands.hpp"
#include "furious/core/command.hpp"
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

TEST_F(ProjectDataTest, PatternWithTriggersRoundTrip) {
    ProjectData original;

    Pattern pattern;
    pattern.id = "test_pattern";
    pattern.name = "Test Pattern";
    pattern.length_subdivisions = 32;

    PatternTrigger trigger1;
    trigger1.subdivision_index = 0;
    trigger1.target = PatternTargetProperty::ScaleX;
    trigger1.value = 1.5f;

    PatternTrigger trigger2;
    trigger2.subdivision_index = 8;
    trigger2.target = PatternTargetProperty::PositionY;
    trigger2.value = -100.0f;

    PatternTrigger trigger3;
    trigger3.subdivision_index = 16;
    trigger3.target = PatternTargetProperty::Rotation;
    trigger3.value = 45.0f;

    pattern.triggers.push_back(trigger1);
    pattern.triggers.push_back(trigger2);
    pattern.triggers.push_back(trigger3);
    original.patterns.push_back(pattern);

    ASSERT_TRUE(original.save_to_file(test_file_.string()));

    ProjectData loaded;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), loaded));

    ASSERT_EQ(loaded.patterns.size(), 1u);
    const auto& loaded_pattern = loaded.patterns[0];
    EXPECT_EQ(loaded_pattern.id, "test_pattern");
    EXPECT_EQ(loaded_pattern.name, "Test Pattern");
    EXPECT_EQ(loaded_pattern.length_subdivisions, 32);
    ASSERT_EQ(loaded_pattern.triggers.size(), 3u);

    EXPECT_EQ(loaded_pattern.triggers[0].subdivision_index, 0);
    EXPECT_EQ(loaded_pattern.triggers[0].target, PatternTargetProperty::ScaleX);
    EXPECT_FLOAT_EQ(loaded_pattern.triggers[0].value, 1.5f);

    EXPECT_EQ(loaded_pattern.triggers[1].subdivision_index, 8);
    EXPECT_EQ(loaded_pattern.triggers[1].target, PatternTargetProperty::PositionY);
    EXPECT_FLOAT_EQ(loaded_pattern.triggers[1].value, -100.0f);

    EXPECT_EQ(loaded_pattern.triggers[2].subdivision_index, 16);
    EXPECT_EQ(loaded_pattern.triggers[2].target, PatternTargetProperty::Rotation);
    EXPECT_FLOAT_EQ(loaded_pattern.triggers[2].value, 45.0f);
}

TEST_F(ProjectDataTest, PatternWithEmptyTriggers) {
    ProjectData original;

    Pattern pattern;
    pattern.id = "empty_pattern";
    pattern.name = "Empty Pattern";
    pattern.length_subdivisions = 16;
    original.patterns.push_back(pattern);

    ASSERT_TRUE(original.save_to_file(test_file_.string()));

    ProjectData loaded;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), loaded));

    ASSERT_EQ(loaded.patterns.size(), 1u);
    EXPECT_EQ(loaded.patterns[0].id, "empty_pattern");
    EXPECT_TRUE(loaded.patterns[0].triggers.empty());
}

TEST_F(ProjectDataTest, PatternAllTargetPropertiesRoundTrip) {
    ProjectData original;

    Pattern pattern;
    pattern.id = "all_props";
    pattern.name = "All Properties";

    std::vector<PatternTargetProperty> props = {
        PatternTargetProperty::PositionX,
        PatternTargetProperty::PositionY,
        PatternTargetProperty::ScaleX,
        PatternTargetProperty::ScaleY,
        PatternTargetProperty::Rotation
    };

    for (size_t i = 0; i < props.size(); ++i) {
        PatternTrigger trigger;
        trigger.subdivision_index = static_cast<int>(i);
        trigger.target = props[i];
        trigger.value = static_cast<float>(i) * 10.0f;
        pattern.triggers.push_back(trigger);
    }

    original.patterns.push_back(pattern);
    ASSERT_TRUE(original.save_to_file(test_file_.string()));

    ProjectData loaded;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), loaded));

    ASSERT_EQ(loaded.patterns.size(), 1u);
    ASSERT_EQ(loaded.patterns[0].triggers.size(), props.size());

    for (size_t i = 0; i < props.size(); ++i) {
        EXPECT_EQ(loaded.patterns[0].triggers[i].target, props[i]);
    }
}

TEST_F(ProjectDataTest, ClipPatternReferenceRoundTrip) {
    ProjectData original;

    Pattern pattern;
    pattern.id = "pat_001";
    pattern.name = "Bounce";
    original.patterns.push_back(pattern);

    TimelineClip clip;
    clip.id = "clip_001";
    clip.source_id = "source_001";

    ClipPatternReference ref1;
    ref1.pattern_id = "pat_001";
    ref1.enabled = true;
    ref1.offset_subdivisions = 4;

    ClipPatternReference ref2;
    ref2.pattern_id = "pat_002";
    ref2.enabled = false;
    ref2.offset_subdivisions = 0;

    clip.patterns.push_back(ref1);
    clip.patterns.push_back(ref2);
    original.clips.push_back(clip);

    ASSERT_TRUE(original.save_to_file(test_file_.string()));

    ProjectData loaded;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), loaded));

    ASSERT_EQ(loaded.clips.size(), 1u);
    ASSERT_EQ(loaded.clips[0].patterns.size(), 2u);

    EXPECT_EQ(loaded.clips[0].patterns[0].pattern_id, "pat_001");
    EXPECT_TRUE(loaded.clips[0].patterns[0].enabled);
    EXPECT_EQ(loaded.clips[0].patterns[0].offset_subdivisions, 4);

    EXPECT_EQ(loaded.clips[0].patterns[1].pattern_id, "pat_002");
    EXPECT_FALSE(loaded.clips[0].patterns[1].enabled);
    EXPECT_EQ(loaded.clips[0].patterns[1].offset_subdivisions, 0);
}

TEST_F(ProjectDataTest, PatternWorkflowSimulation) {
    PatternLibrary library;
    CommandHistory history;

    Pattern pattern;
    pattern.id = PatternLibrary::generate_id();
    pattern.name = "Test Pattern";
    pattern.length_subdivisions = 16;

    auto add_cmd = std::make_unique<AddPatternCommand>(library, pattern);
    history.execute(std::move(add_cmd));

    ASSERT_EQ(library.patterns().size(), 1u);
    EXPECT_TRUE(library.patterns()[0].triggers.empty());

    Pattern* lib_pattern = library.find_pattern(pattern.id);
    ASSERT_NE(lib_pattern, nullptr);

    std::vector<PatternTargetProperty> props = {
        PatternTargetProperty::PositionX,
        PatternTargetProperty::PositionY,
        PatternTargetProperty::ScaleX,
        PatternTargetProperty::ScaleY,
        PatternTargetProperty::Rotation
    };

    for (int i = 0; i < 16; ++i) {
        Pattern old_state = *lib_pattern;

        PatternTrigger trigger;
        trigger.subdivision_index = i;
        trigger.target = props[i % props.size()];
        trigger.value = static_cast<float>(i) * 0.1f + 0.5f;
        lib_pattern->triggers.push_back(trigger);

        auto modify_cmd = std::make_unique<ModifyPatternCommand>(
            library, lib_pattern->id, old_state, *lib_pattern, "Add trigger");
        history.execute(std::move(modify_cmd));
    }

    EXPECT_EQ(library.patterns()[0].triggers.size(), 16u);

    ProjectData data;
    data.patterns = library.patterns();

    ASSERT_TRUE(data.save_to_file(test_file_.string()));

    ProjectData loaded;
    ASSERT_TRUE(ProjectData::load_from_file(test_file_.string(), loaded));

    ASSERT_EQ(loaded.patterns.size(), 1u);
    ASSERT_EQ(loaded.patterns[0].triggers.size(), 16u);

    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(loaded.patterns[0].triggers[i].subdivision_index, i);
        EXPECT_EQ(loaded.patterns[0].triggers[i].target, props[i % props.size()]);
        EXPECT_FLOAT_EQ(loaded.patterns[0].triggers[i].value, static_cast<float>(i) * 0.1f + 0.5f);
    }
}
