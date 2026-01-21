#include "furious/scripting/script_engine.hpp"
#include "furious/core/tempo.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace furious;

TEST(ScriptEngineTest, InitializeAndShutdown) {
    ScriptEngine engine;
    EXPECT_FALSE(engine.is_initialized());

    EXPECT_TRUE(engine.initialize());
    EXPECT_TRUE(engine.is_initialized());

    engine.shutdown();
    EXPECT_FALSE(engine.is_initialized());
}

TEST(ScriptEngineTest, MultipleInitializationsAreSafe) {
    ScriptEngine engine;
    EXPECT_TRUE(engine.initialize());
    EXPECT_TRUE(engine.initialize());
    EXPECT_TRUE(engine.is_initialized());
}

TEST(ScriptEngineTest, AvailableEffectsEmptyInitially) {
    ScriptEngine engine;
    engine.initialize();

    EXPECT_TRUE(engine.available_effects().empty());
}

TEST(ScriptEngineTest, ScanNonexistentDirectoryIsNoOp) {
    ScriptEngine engine;
    engine.initialize();
    engine.add_effect_directory("/nonexistent/path/to/effects");
    engine.scan_effect_directories();

    EXPECT_TRUE(engine.available_effects().empty());
}

TEST(ScriptEngineTest, FindEffectReturnsNullForUnknown) {
    ScriptEngine engine;
    engine.initialize();

    EXPECT_EQ(engine.find_effect("nonexistent"), nullptr);
}

TEST(EffectResultTest, DefaultValues) {
    EffectResult result;
    EXPECT_DOUBLE_EQ(result.source_position_seconds, 0.0);
    EXPECT_FALSE(result.use_looped_frame);
    EXPECT_DOUBLE_EQ(result.loop_start_seconds, 0.0);
    EXPECT_DOUBLE_EQ(result.loop_duration_seconds, 0.0);
    EXPECT_DOUBLE_EQ(result.position_in_loop_seconds, 0.0);
    EXPECT_FALSE(result.position_x.has_value());
    EXPECT_FALSE(result.position_y.has_value());
    EXPECT_FALSE(result.scale_x.has_value());
    EXPECT_FALSE(result.scale_y.has_value());
    EXPECT_FALSE(result.rotation.has_value());
}

TEST(EffectContextTest, DefaultValues) {
    EffectContext context;
    EXPECT_EQ(context.clip, nullptr);
    EXPECT_EQ(context.tempo, nullptr);
    EXPECT_DOUBLE_EQ(context.current_beats, 0.0);
    EXPECT_DOUBLE_EQ(context.clip_local_beats, 0.0);
}

TEST(EffectContextTest, CanSetValues) {
    TimelineClip clip;
    clip.start_beat = 4.0;
    clip.duration_beats = 8.0;

    Tempo tempo(120.0);

    EffectContext context;
    context.clip = &clip;
    context.tempo = &tempo;
    context.current_beats = 6.0;
    context.clip_local_beats = 2.0;

    EXPECT_EQ(context.clip, &clip);
    EXPECT_EQ(context.tempo, &tempo);
    EXPECT_DOUBLE_EQ(context.current_beats, 6.0);
    EXPECT_DOUBLE_EQ(context.clip_local_beats, 2.0);
}

TEST(ScriptEngineTest, EvaluateUnknownEffectReturnsDefault) {
    ScriptEngine engine;
    engine.initialize();

    ClipEffect effect;
    effect.effect_id = "nonexistent_effect";

    EffectContext context;

    EffectResult result = engine.evaluate_effect(effect, context);

    EXPECT_FALSE(result.use_looped_frame);
}

TEST(ScriptEngineTest, EvaluateEmptyEffectsListReturnsDefault) {
    ScriptEngine engine;
    engine.initialize();

    std::vector<ClipEffect> effects;
    EffectContext context;

    EffectResult result = engine.evaluate_effects(effects, context);

    EXPECT_FALSE(result.use_looped_frame);
}

TEST(ScriptEngineTest, EvaluateDisabledEffectSkipsIt) {
    ScriptEngine engine;
    engine.initialize();

    ClipEffect effect;
    effect.effect_id = "test";
    effect.enabled = false;

    std::vector<ClipEffect> effects = {effect};
    EffectContext context;

    EffectResult result = engine.evaluate_effects(effects, context);

    EXPECT_FALSE(result.use_looped_frame);
}

