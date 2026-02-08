#include <gtest/gtest.h>
#include "furious/core/pattern_evaluator.hpp"
#include "furious/core/pattern_library.hpp"
#include "furious/core/timeline_clip.hpp"

namespace furious {
namespace {

class PatternEvaluatorTest : public ::testing::Test {
protected:
    PatternLibrary library;
    PatternEvaluator evaluator;
    TimelineClip clip;

    void SetUp() override {
        evaluator.set_pattern_library(&library);
        clip.id = "test_clip";
    }

    std::string create_pattern_with_trigger(PatternTargetProperty prop, int subdivision, float value) {
        std::string id = library.create_pattern("Test");
        Pattern* p = library.find_pattern(id);
        p->triggers.push_back({subdivision, prop, value});
        return id;
    }
};

TEST_F(PatternEvaluatorTest, EvaluateWithNoLibraryReturnsEmptyResult) {
    PatternEvaluator no_lib_evaluator;
    clip.patterns.push_back({"some_pattern", true, 0});

    auto result = no_lib_evaluator.evaluate(clip, 0.0);

    EXPECT_FALSE(result.position_x.has_value());
    EXPECT_FALSE(result.scale_x.has_value());
    EXPECT_FALSE(result.restart_clip);
}

TEST_F(PatternEvaluatorTest, EvaluateWithNoPatternsReturnsEmptyResult) {
    auto result = evaluator.evaluate(clip, 0.0);

    EXPECT_FALSE(result.position_x.has_value());
    EXPECT_FALSE(result.position_y.has_value());
    EXPECT_FALSE(result.scale_x.has_value());
    EXPECT_FALSE(result.scale_y.has_value());
    EXPECT_FALSE(result.rotation.has_value());
    EXPECT_FALSE(result.flip_h.has_value());
    EXPECT_FALSE(result.flip_v.has_value());
    EXPECT_FALSE(result.restart_clip);
}

TEST_F(PatternEvaluatorTest, EvaluateDisabledPatternIsIgnored) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::ScaleX, 0, 2.0f);
    clip.patterns.push_back({id, false, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    EXPECT_FALSE(result.scale_x.has_value());
}

TEST_F(PatternEvaluatorTest, EvaluateNonexistentPatternIsIgnored) {
    clip.patterns.push_back({"nonexistent_id", true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    EXPECT_FALSE(result.scale_x.has_value());
}

TEST_F(PatternEvaluatorTest, EvaluateScaleXAtSubdivisionZero) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::ScaleX, 0, 2.0f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 2.0f);
}

TEST_F(PatternEvaluatorTest, EvaluatePositionX) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::PositionX, 0, 100.0f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.position_x.has_value());
    EXPECT_FLOAT_EQ(*result.position_x, 100.0f);
}

TEST_F(PatternEvaluatorTest, EvaluatePositionY) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::PositionY, 0, -50.0f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.position_y.has_value());
    EXPECT_FLOAT_EQ(*result.position_y, -50.0f);
}

TEST_F(PatternEvaluatorTest, EvaluateScaleY) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::ScaleY, 0, 0.5f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.scale_y.has_value());
    EXPECT_FLOAT_EQ(*result.scale_y, 0.5f);
}

TEST_F(PatternEvaluatorTest, EvaluateRotation) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::Rotation, 0, 45.0f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.rotation.has_value());
    EXPECT_FLOAT_EQ(*result.rotation, 45.0f);
}

TEST_F(PatternEvaluatorTest, EvaluateFlipHTrue) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::FlipH, 0, 1.0f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.flip_h.has_value());
    EXPECT_TRUE(*result.flip_h);
}

TEST_F(PatternEvaluatorTest, EvaluateFlipHFalse) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::FlipH, 0, 0.0f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.flip_h.has_value());
    EXPECT_FALSE(*result.flip_h);
}

TEST_F(PatternEvaluatorTest, EvaluateFlipV) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::FlipV, 0, 1.0f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.flip_v.has_value());
    EXPECT_TRUE(*result.flip_v);
}

TEST_F(PatternEvaluatorTest, SubdivisionCalculation4PerBeat) {
    std::string id = library.create_pattern("Test");
    Pattern* p = library.find_pattern(id);
    p->triggers.push_back({4, PatternTargetProperty::ScaleX, 3.0f});
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 1.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 3.0f);
}

TEST_F(PatternEvaluatorTest, SubdivisionWrapsAroundPatternLength) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::ScaleX, 0, 2.0f);
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 4.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 2.0f);
}

TEST_F(PatternEvaluatorTest, HeldPropertyPersistsBetweenTriggers) {
    std::string id = library.create_pattern("Test");
    Pattern* p = library.find_pattern(id);
    p->triggers.push_back({0, PatternTargetProperty::ScaleX, 2.0f});
    p->triggers.push_back({8, PatternTargetProperty::ScaleX, 1.0f});
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 1.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 2.0f);

    result = evaluator.evaluate(clip, 2.0);
    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 1.0f);
}

TEST_F(PatternEvaluatorTest, HeldPropertyWrapsAroundCircularly) {
    std::string id = library.create_pattern("Test");
    Pattern* p = library.find_pattern(id);
    p->triggers.push_back({12, PatternTargetProperty::ScaleX, 5.0f});
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 5.0f);
}

TEST_F(PatternEvaluatorTest, OffsetAdjustsSubdivision) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::ScaleX, 4, 7.0f);
    clip.patterns.push_back({id, true, 4});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 7.0f);
}

TEST_F(PatternEvaluatorTest, NegativeOffsetHandledCorrectly) {
    std::string id = create_pattern_with_trigger(PatternTargetProperty::ScaleX, 12, 4.0f);
    clip.patterns.push_back({id, true, -4});

    auto result = evaluator.evaluate(clip, 4.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 4.0f);
}

TEST_F(PatternEvaluatorTest, MultiplePatternsCombine) {
    std::string id1 = create_pattern_with_trigger(PatternTargetProperty::ScaleX, 0, 2.0f);
    std::string id2 = create_pattern_with_trigger(PatternTargetProperty::ScaleY, 0, 3.0f);

    clip.patterns.push_back({id1, true, 0});
    clip.patterns.push_back({id2, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 2.0f);
    ASSERT_TRUE(result.scale_y.has_value());
    EXPECT_FLOAT_EQ(*result.scale_y, 3.0f);
}

TEST_F(PatternEvaluatorTest, LaterPatternOverridesSameProperty) {
    std::string id1 = create_pattern_with_trigger(PatternTargetProperty::ScaleX, 0, 2.0f);
    std::string id2 = create_pattern_with_trigger(PatternTargetProperty::ScaleX, 0, 5.0f);

    clip.patterns.push_back({id1, true, 0});
    clip.patterns.push_back({id2, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 5.0f);
}

TEST_F(PatternEvaluatorTest, EmptyPatternIsIgnored) {
    std::string id = library.create_pattern("Empty");
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    EXPECT_FALSE(result.scale_x.has_value());
}

TEST_F(PatternEvaluatorTest, AllPropertiesCanBeSet) {
    std::string id = library.create_pattern("All Props");
    Pattern* p = library.find_pattern(id);
    p->triggers.push_back({0, PatternTargetProperty::PositionX, 10.0f});
    p->triggers.push_back({0, PatternTargetProperty::PositionY, 20.0f});
    p->triggers.push_back({0, PatternTargetProperty::ScaleX, 2.0f});
    p->triggers.push_back({0, PatternTargetProperty::ScaleY, 3.0f});
    p->triggers.push_back({0, PatternTargetProperty::Rotation, 90.0f});
    p->triggers.push_back({0, PatternTargetProperty::FlipH, 1.0f});
    p->triggers.push_back({0, PatternTargetProperty::FlipV, 1.0f});

    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);

    ASSERT_TRUE(result.position_x.has_value());
    EXPECT_FLOAT_EQ(*result.position_x, 10.0f);
    ASSERT_TRUE(result.position_y.has_value());
    EXPECT_FLOAT_EQ(*result.position_y, 20.0f);
    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 2.0f);
    ASSERT_TRUE(result.scale_y.has_value());
    EXPECT_FLOAT_EQ(*result.scale_y, 3.0f);
    ASSERT_TRUE(result.rotation.has_value());
    EXPECT_FLOAT_EQ(*result.rotation, 90.0f);
    ASSERT_TRUE(result.flip_h.has_value());
    EXPECT_TRUE(*result.flip_h);
    ASSERT_TRUE(result.flip_v.has_value());
    EXPECT_TRUE(*result.flip_v);
}

TEST_F(PatternEvaluatorTest, FractionalBeatSubdivisionCalculation) {
    std::string id = library.create_pattern("Test");
    Pattern* p = library.find_pattern(id);
    p->triggers.push_back({1, PatternTargetProperty::ScaleX, 1.5f});
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.25);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 1.5f);
}

TEST_F(PatternEvaluatorTest, VeryLongPatternLength) {
    std::string id = library.create_pattern("Long");
    Pattern* p = library.find_pattern(id);
    p->length_subdivisions = 64;
    p->triggers.push_back({32, PatternTargetProperty::ScaleX, 4.0f});
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 8.0);

    ASSERT_TRUE(result.scale_x.has_value());
    EXPECT_FLOAT_EQ(*result.scale_x, 4.0f);
}

TEST_F(PatternEvaluatorTest, ShortPatternLength) {
    std::string id = library.create_pattern("Short");
    Pattern* p = library.find_pattern(id);
    p->length_subdivisions = 4;
    p->triggers.push_back({0, PatternTargetProperty::ScaleX, 2.0f});
    p->triggers.push_back({2, PatternTargetProperty::ScaleX, 3.0f});
    clip.patterns.push_back({id, true, 0});

    EXPECT_FLOAT_EQ(*evaluator.evaluate(clip, 0.0).scale_x, 2.0f);

    EXPECT_FLOAT_EQ(*evaluator.evaluate(clip, 0.5).scale_x, 3.0f);

    EXPECT_FLOAT_EQ(*evaluator.evaluate(clip, 1.0).scale_x, 2.0f);
}

TEST_F(PatternEvaluatorTest, RestartOnTriggerSetsRestartClipAtTriggerSubdivision) {
    std::string id = library.create_pattern("Restart");
    Pattern* p = library.find_pattern(id);
    p->restart_on_trigger = true;
    p->triggers.push_back({0, PatternTargetProperty::ScaleX, 1.0f});
    p->triggers.push_back({4, PatternTargetProperty::ScaleX, 2.0f});
    clip.patterns.push_back({id, true, 0});

    auto result0 = evaluator.evaluate(clip, 0.0);
    EXPECT_TRUE(result0.restart_clip);

    auto result1 = evaluator.evaluate(clip, 0.25);
    EXPECT_FALSE(result1.restart_clip);

    auto result4 = evaluator.evaluate(clip, 1.0);
    EXPECT_TRUE(result4.restart_clip);
}

TEST_F(PatternEvaluatorTest, RestartOnTriggerDisabledDoesNotSetRestartClip) {
    std::string id = library.create_pattern("NoRestart");
    Pattern* p = library.find_pattern(id);
    p->restart_on_trigger = false;
    p->triggers.push_back({0, PatternTargetProperty::ScaleX, 1.0f});
    clip.patterns.push_back({id, true, 0});

    auto result = evaluator.evaluate(clip, 0.0);
    EXPECT_FALSE(result.restart_clip);
}

} // namespace
} // namespace furious
