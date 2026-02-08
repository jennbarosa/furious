#pragma once

#include "furious/core/timeline_clip.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace furious {

class Project;
class TimelineData;
class Tempo;

struct EffectParameter {
    std::string name;
    std::string type;
    std::string default_value;
    std::vector<std::string> enum_values;
};

struct EffectInfo {
    std::string id;
    std::string name;
    std::string script_path;
    std::vector<EffectParameter> parameters;
};

struct EffectResult {
    double source_position_seconds = 0.0;
    bool use_looped_frame = false;
    double loop_start_seconds = 0.0;
    double loop_duration_seconds = 0.0;
    double position_in_loop_seconds = 0.0;

    bool use_looped_audio = false;
    double audio_loop_start_seconds = 0.0;
    double audio_loop_duration_seconds = 0.0;

    std::optional<float> position_x;
    std::optional<float> position_y;
    std::optional<float> scale_x;
    std::optional<float> scale_y;
    std::optional<float> rotation;
};

struct AccumulatedTransform {
    float position_x = 0.0f;
    float position_y = 0.0f;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    float rotation = 0.0f;
};

struct EffectContext {
    const TimelineClip* clip = nullptr;
    const Tempo* tempo = nullptr;
    double current_beats = 0.0;
    double clip_local_beats = 0.0;
    AccumulatedTransform accumulated;
};

class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    bool initialize();
    void shutdown();

    void scan_effect_directories();
    void add_effect_directory(const std::string& path);
    [[nodiscard]] const std::vector<EffectInfo>& available_effects() const;
    [[nodiscard]] const EffectInfo* find_effect(const std::string& effect_id) const;

    void set_project(Project* project);
    void set_timeline_data(TimelineData* data);

    EffectResult evaluate_effect(
        const ClipEffect& effect,
        const EffectContext& context
    );

    EffectResult evaluate_effects(
        const std::vector<ClipEffect>& effects,
        const EffectContext& context
    );

    [[nodiscard]] bool is_initialized() const;
    [[nodiscard]] const std::string& last_error() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace furious
