#include <gtest/gtest.h>
#include "furious/core/pattern.hpp"
#include "furious/core/pattern_library.hpp"

namespace furious {
namespace {

class PatternTest : public ::testing::Test {
protected:
    Pattern pattern;

    void SetUp() override {
        pattern.id = "test_pattern";
        pattern.name = "Test Pattern";
        pattern.length_subdivisions = 16;
    }
};

TEST_F(PatternTest, DefaultState) {
    Pattern p;
    EXPECT_TRUE(p.id.empty());
    EXPECT_TRUE(p.name.empty());
    EXPECT_EQ(p.length_subdivisions, 16);
    EXPECT_TRUE(p.triggers.empty());
    EXPECT_FALSE(p.scale_x_settings.restart_on_trigger);
    EXPECT_FALSE(p.position_x_settings.restart_on_trigger);
}

TEST_F(PatternTest, ValueAtReturnsNulloptWhenNoTriggers) {
    auto result = pattern.value_at(0, PatternTargetProperty::ScaleX);
    EXPECT_FALSE(result.has_value());
}

TEST_F(PatternTest, ValueAtReturnsTriggerValue) {
    pattern.triggers.push_back({0, PatternTargetProperty::ScaleX, 2.0f});
    auto result = pattern.value_at(0, PatternTargetProperty::ScaleX);
    ASSERT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(*result, 2.0f);
}

TEST_F(PatternTest, ValueAtReturnsNulloptForDifferentProperty) {
    pattern.triggers.push_back({0, PatternTargetProperty::ScaleX, 2.0f});
    auto result = pattern.value_at(0, PatternTargetProperty::ScaleY);
    EXPECT_FALSE(result.has_value());
}

TEST_F(PatternTest, ValueAtReturnsNulloptForDifferentSubdivision) {
    pattern.triggers.push_back({0, PatternTargetProperty::ScaleX, 2.0f});
    auto result = pattern.value_at(1, PatternTargetProperty::ScaleX);
    EXPECT_FALSE(result.has_value());
}

TEST_F(PatternTest, ValueAtWithMultipleTriggers) {
    pattern.triggers.push_back({0, PatternTargetProperty::ScaleX, 1.0f});
    pattern.triggers.push_back({4, PatternTargetProperty::ScaleX, 2.0f});
    pattern.triggers.push_back({8, PatternTargetProperty::ScaleY, 3.0f});

    EXPECT_FLOAT_EQ(*pattern.value_at(0, PatternTargetProperty::ScaleX), 1.0f);
    EXPECT_FLOAT_EQ(*pattern.value_at(4, PatternTargetProperty::ScaleX), 2.0f);
    EXPECT_FLOAT_EQ(*pattern.value_at(8, PatternTargetProperty::ScaleY), 3.0f);
    EXPECT_FALSE(pattern.value_at(8, PatternTargetProperty::ScaleX).has_value());
}

class PatternTriggerTest : public ::testing::Test {};

TEST_F(PatternTriggerTest, DefaultValues) {
    PatternTrigger trigger;
    EXPECT_EQ(trigger.subdivision_index, 0);
    EXPECT_EQ(trigger.target, PatternTargetProperty::ScaleX);
    EXPECT_FLOAT_EQ(trigger.value, 1.0f);
}

TEST_F(PatternTriggerTest, CustomValues) {
    PatternTrigger trigger{8, PatternTargetProperty::Rotation, 45.0f};
    EXPECT_EQ(trigger.subdivision_index, 8);
    EXPECT_EQ(trigger.target, PatternTargetProperty::Rotation);
    EXPECT_FLOAT_EQ(trigger.value, 45.0f);
}

class ClipPatternReferenceTest : public ::testing::Test {};

TEST_F(ClipPatternReferenceTest, DefaultValues) {
    ClipPatternReference ref;
    EXPECT_TRUE(ref.pattern_id.empty());
    EXPECT_TRUE(ref.enabled);
    EXPECT_EQ(ref.offset_subdivisions, 0);
}

TEST_F(ClipPatternReferenceTest, CustomValues) {
    ClipPatternReference ref{"pattern_1", false, 4};
    EXPECT_EQ(ref.pattern_id, "pattern_1");
    EXPECT_FALSE(ref.enabled);
    EXPECT_EQ(ref.offset_subdivisions, 4);
}

class PatternLibraryTest : public ::testing::Test {
protected:
    PatternLibrary library;
};

TEST_F(PatternLibraryTest, InitiallyEmpty) {
    EXPECT_EQ(library.pattern_count(), 0u);
    EXPECT_TRUE(library.patterns().empty());
}

TEST_F(PatternLibraryTest, CreatePatternReturnsId) {
    std::string id = library.create_pattern("My Pattern");
    EXPECT_FALSE(id.empty());
    EXPECT_EQ(library.pattern_count(), 1u);
}

TEST_F(PatternLibraryTest, CreatePatternSetsName) {
    std::string id = library.create_pattern("My Pattern");
    const Pattern* p = library.find_pattern(id);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name, "My Pattern");
}

TEST_F(PatternLibraryTest, FindPatternReturnsNullForMissingId) {
    EXPECT_EQ(library.find_pattern("nonexistent"), nullptr);
}

TEST_F(PatternLibraryTest, AddPattern) {
    Pattern p;
    p.id = "custom_id";
    p.name = "Custom Pattern";
    library.add_pattern(p);

    const Pattern* found = library.find_pattern("custom_id");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "Custom Pattern");
}

TEST_F(PatternLibraryTest, RemovePattern) {
    std::string id = library.create_pattern("To Remove");
    EXPECT_EQ(library.pattern_count(), 1u);

    library.remove_pattern(id);
    EXPECT_EQ(library.pattern_count(), 0u);
    EXPECT_EQ(library.find_pattern(id), nullptr);
}

TEST_F(PatternLibraryTest, RemoveNonexistentPatternDoesNothing) {
    library.create_pattern("Keep");
    EXPECT_EQ(library.pattern_count(), 1u);

    library.remove_pattern("nonexistent");
    EXPECT_EQ(library.pattern_count(), 1u);
}

TEST_F(PatternLibraryTest, DuplicatePattern) {
    std::string original_id = library.create_pattern("Original");
    Pattern* original = library.find_pattern(original_id);
    original->triggers.push_back({0, PatternTargetProperty::ScaleX, 2.0f});

    std::string copy_id = library.duplicate_pattern(original_id);
    EXPECT_FALSE(copy_id.empty());
    EXPECT_NE(copy_id, original_id);
    EXPECT_EQ(library.pattern_count(), 2u);

    const Pattern* copy = library.find_pattern(copy_id);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->triggers.size(), 1u);
    EXPECT_FLOAT_EQ(copy->triggers[0].value, 2.0f);
}

TEST_F(PatternLibraryTest, DuplicateNonexistentPatternReturnsEmpty) {
    std::string id = library.duplicate_pattern("nonexistent");
    EXPECT_TRUE(id.empty());
}

TEST_F(PatternLibraryTest, Clear) {
    library.create_pattern("Pattern 1");
    library.create_pattern("Pattern 2");
    EXPECT_EQ(library.pattern_count(), 2u);

    library.clear();
    EXPECT_EQ(library.pattern_count(), 0u);
}

TEST_F(PatternLibraryTest, GenerateIdProducesUniqueIds) {
    std::string id1 = PatternLibrary::generate_id();
    std::string id2 = PatternLibrary::generate_id();
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

TEST_F(PatternLibraryTest, FindPatternMutable) {
    std::string id = library.create_pattern("Mutable Test");
    Pattern* p = library.find_pattern(id);
    ASSERT_NE(p, nullptr);

    p->name = "Modified Name";
    EXPECT_EQ(library.find_pattern(id)->name, "Modified Name");
}

} // namespace
} // namespace furious
