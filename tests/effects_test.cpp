#include "furious/core/timeline_clip.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace furious;

TEST(ClipEffectTest, DefaultValues) {
    ClipEffect effect;
    EXPECT_TRUE(effect.effect_id.empty());
    EXPECT_TRUE(effect.parameters.empty());
    EXPECT_TRUE(effect.enabled);
}

TEST(ClipEffectTest, CanSetEffectId) {
    ClipEffect effect;
    effect.effect_id = "auto_ytpmv";
    EXPECT_EQ(effect.effect_id, "auto_ytpmv");
}

TEST(ClipEffectTest, CanSetParameters) {
    ClipEffect effect;
    effect.parameters["sync_period"] = "quarter";
    effect.parameters["intensity"] = "0.5";

    EXPECT_EQ(effect.parameters.size(), 2);
    EXPECT_EQ(effect.parameters["sync_period"], "quarter");
    EXPECT_EQ(effect.parameters["intensity"], "0.5");
}

TEST(ClipEffectTest, CanDisableEffect) {
    ClipEffect effect;
    EXPECT_TRUE(effect.enabled);

    effect.enabled = false;
    EXPECT_FALSE(effect.enabled);
}

TEST(TimelineClipTest, EffectsVectorEmptyByDefault) {
    TimelineClip clip;
    EXPECT_TRUE(clip.effects.empty());
}

TEST(TimelineClipTest, CanAddSingleEffect) {
    TimelineClip clip;

    ClipEffect effect;
    effect.effect_id = "auto_ytpmv";
    effect.parameters["sync_period"] = "quarter";
    clip.effects.push_back(effect);

    EXPECT_EQ(clip.effects.size(), 1);
    EXPECT_EQ(clip.effects[0].effect_id, "auto_ytpmv");
    EXPECT_EQ(clip.effects[0].parameters["sync_period"], "quarter");
}

TEST(TimelineClipTest, CanAddMultipleEffects) {
    TimelineClip clip;

    ClipEffect effect1;
    effect1.effect_id = "auto_ytpmv";
    effect1.parameters["sync_period"] = "eighth";
    clip.effects.push_back(effect1);

    ClipEffect effect2;
    effect2.effect_id = "shake";
    effect2.parameters["intensity"] = "0.5";
    clip.effects.push_back(effect2);

    EXPECT_EQ(clip.effects.size(), 2);
    EXPECT_EQ(clip.effects[0].effect_id, "auto_ytpmv");
    EXPECT_EQ(clip.effects[1].effect_id, "shake");
}

TEST(TimelineClipTest, EffectsChainOrdering) {
    TimelineClip clip;

    for (int i = 0; i < 5; ++i) {
        ClipEffect effect;
        effect.effect_id = "effect_" + std::to_string(i);
        clip.effects.push_back(effect);
    }

    EXPECT_EQ(clip.effects.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(clip.effects[i].effect_id, "effect_" + std::to_string(i));
    }
}

TEST(TimelineClipTest, CanRemoveEffect) {
    TimelineClip clip;

    ClipEffect effect;
    effect.effect_id = "test";
    clip.effects.push_back(effect);

    EXPECT_EQ(clip.effects.size(), 1);

    clip.effects.clear();
    EXPECT_TRUE(clip.effects.empty());
}

TEST(TimelineClipTest, CanToggleEffectEnabled) {
    TimelineClip clip;

    ClipEffect effect;
    effect.effect_id = "auto_ytpmv";
    effect.enabled = true;
    clip.effects.push_back(effect);

    EXPECT_TRUE(clip.effects[0].enabled);

    clip.effects[0].enabled = false;
    EXPECT_FALSE(clip.effects[0].enabled);

    clip.effects[0].enabled = true;
    EXPECT_TRUE(clip.effects[0].enabled);
}

TEST(TimelineClipTest, EffectParametersCanBeModified) {
    TimelineClip clip;

    ClipEffect effect;
    effect.effect_id = "auto_ytpmv";
    effect.parameters["sync_period"] = "quarter";
    clip.effects.push_back(effect);

    EXPECT_EQ(clip.effects[0].parameters["sync_period"], "quarter");

    clip.effects[0].parameters["sync_period"] = "sixteenth";
    EXPECT_EQ(clip.effects[0].parameters["sync_period"], "sixteenth");
}

TEST(TimelineClipTest, MultipleClipsWithDifferentEffects) {
    TimelineClip clip1;
    clip1.id = "clip-1";

    ClipEffect effect1;
    effect1.effect_id = "auto_ytpmv";
    effect1.parameters["sync_period"] = "quarter";
    clip1.effects.push_back(effect1);

    TimelineClip clip2;
    clip2.id = "clip-2";

    ClipEffect effect2;
    effect2.effect_id = "auto_ytpmv";
    effect2.parameters["sync_period"] = "eighth";
    clip2.effects.push_back(effect2);

    EXPECT_EQ(clip1.effects[0].parameters["sync_period"], "quarter");
    EXPECT_EQ(clip2.effects[0].parameters["sync_period"], "eighth");
}

TEST(TimelineClipTest, ClipDurationIndependentOfEffects) {
    TimelineClip clip;
    clip.duration_beats = 8.0;

    ClipEffect effect;
    effect.effect_id = "auto_ytpmv";
    effect.parameters["sync_period"] = "quarter";
    clip.effects.push_back(effect);

    EXPECT_DOUBLE_EQ(clip.duration_beats, 8.0);

    clip.effects[0].parameters["sync_period"] = "eighth";
    EXPECT_DOUBLE_EQ(clip.duration_beats, 8.0);

    clip.effects[0].parameters["sync_period"] = "measure";
    EXPECT_DOUBLE_EQ(clip.duration_beats, 8.0);
}
