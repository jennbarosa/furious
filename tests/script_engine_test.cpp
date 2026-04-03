#include "furious/scripting/script_engine.hpp"
#include "furious/core/tempo.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <fstream>
#include <cstdio>
#include <filesystem>

using namespace furious;

// Create a unique temp directory and write a Lua file into it.
// Returns {directory, filepath} so the caller can pass the directory
// to add_effect_directory and clean up both after.
struct TempLuaFile {
    std::string dir;
    std::string path;

    ~TempLuaFile() {
        if (!path.empty()) std::filesystem::remove(path);
        if (!dir.empty()) std::filesystem::remove(dir);
    }
};

static TempLuaFile write_temp_lua(const std::string& name, const std::string& content) {
    auto dir = std::filesystem::temp_directory_path() / ("furious_test_" + name + "_" + std::to_string(reinterpret_cast<uintptr_t>(&content)));
    std::filesystem::create_directories(dir);
    auto filepath = dir / (name + ".lua");
    std::ofstream f(filepath);
    f << content;
    f.close();
    return {dir.string(), filepath.string()};
}

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

// --- Lua sandbox tests ---

TEST(LuaSandboxTest, DofileIsBlocked) {
    ScriptEngine engine;
    engine.initialize();

    auto tmp = write_temp_lua("dofile_test", R"(
effect = { id = "dofile_test", name = "dofile test" }
function evaluate(ctx, params)
    if dofile ~= nil then
        return { dofile_exists = 1 }
    end
    return {}
end
)");

    engine.add_effect_directory(tmp.dir);
    engine.scan_effect_directories();
    ASSERT_NE(engine.find_effect("dofile_test"), nullptr);

    ClipEffect ce;
    ce.effect_id = "dofile_test";
    ce.enabled = true;
    engine.evaluate_effect(ce, EffectContext{});
}

TEST(LuaSandboxTest, LoadIsBlocked) {
    ScriptEngine engine;
    engine.initialize();

    auto tmp = write_temp_lua("load_test", R"(
effect = { id = "load_test", name = "load test" }
function evaluate(ctx, params)
    if load ~= nil then
        return { load_exists = 1 }
    end
    return {}
end
)");

    engine.add_effect_directory(tmp.dir);
    engine.scan_effect_directories();
    ASSERT_NE(engine.find_effect("load_test"), nullptr);

    ClipEffect ce;
    ce.effect_id = "load_test";
    ce.enabled = true;
    engine.evaluate_effect(ce, EffectContext{});
}

TEST(LuaSandboxTest, LoadfileIsBlocked) {
    ScriptEngine engine;
    engine.initialize();

    auto tmp = write_temp_lua("loadfile_test", R"(
effect = { id = "loadfile_test", name = "loadfile test" }
function evaluate(ctx, params)
    if loadfile ~= nil then
        return { loadfile_exists = 1 }
    end
    return {}
end
)");

    engine.add_effect_directory(tmp.dir);
    engine.scan_effect_directories();
    ASSERT_NE(engine.find_effect("loadfile_test"), nullptr);

    ClipEffect ce;
    ce.effect_id = "loadfile_test";
    ce.enabled = true;
    engine.evaluate_effect(ce, EffectContext{});
}

TEST(LuaSandboxTest, OsLibraryNotAvailable) {
    ScriptEngine engine;
    engine.initialize();

    auto tmp = write_temp_lua("os_test", R"(
effect = { id = "os_test", name = "os test" }
function evaluate(ctx, params)
    if os ~= nil then
        return { os_exists = 1 }
    end
    return {}
end
)");

    engine.add_effect_directory(tmp.dir);
    engine.scan_effect_directories();
    ASSERT_NE(engine.find_effect("os_test"), nullptr);

    ClipEffect ce;
    ce.effect_id = "os_test";
    ce.enabled = true;
    engine.evaluate_effect(ce, EffectContext{});
}

TEST(LuaSandboxTest, IoLibraryNotAvailable) {
    ScriptEngine engine;
    engine.initialize();

    auto tmp = write_temp_lua("io_test", R"(
effect = { id = "io_test", name = "io test" }
function evaluate(ctx, params)
    if io ~= nil then
        return { io_exists = 1 }
    end
    return {}
end
)");

    engine.add_effect_directory(tmp.dir);
    engine.scan_effect_directories();
    ASSERT_NE(engine.find_effect("io_test"), nullptr);

    ClipEffect ce;
    ce.effect_id = "io_test";
    ce.enabled = true;
    engine.evaluate_effect(ce, EffectContext{});
}

TEST(LuaSandboxTest, InfiniteLoopIsTerminated) {
    ScriptEngine engine;
    engine.initialize();

    auto tmp = write_temp_lua("loop_test", R"(
effect = { id = "loop_test", name = "loop test" }
function evaluate(ctx, params)
    while true do end
    return {}
end
)");

    engine.add_effect_directory(tmp.dir);
    engine.scan_effect_directories();
    ASSERT_NE(engine.find_effect("loop_test"), nullptr);

    ClipEffect ce;
    ce.effect_id = "loop_test";
    ce.enabled = true;

    EffectResult result = engine.evaluate_effect(ce, EffectContext{});
    EXPECT_FALSE(engine.last_error().empty());
}

TEST(LuaSandboxTest, CollectgarbageIsBlocked) {
    ScriptEngine engine;
    engine.initialize();

    auto tmp = write_temp_lua("gc_test", R"(
effect = { id = "gc_test", name = "gc test" }
function evaluate(ctx, params)
    if collectgarbage ~= nil then
        return { gc_exists = 1 }
    end
    return {}
end
)");

    engine.add_effect_directory(tmp.dir);
    engine.scan_effect_directories();
    ASSERT_NE(engine.find_effect("gc_test"), nullptr);

    ClipEffect ce;
    ce.effect_id = "gc_test";
    ce.enabled = true;
    engine.evaluate_effect(ce, EffectContext{});
}

