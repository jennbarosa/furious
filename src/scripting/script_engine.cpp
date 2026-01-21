#include "furious/scripting/script_engine.hpp"
#include "furious/scripting/lua_bindings.hpp"
#include "furious/core/project.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/tempo.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace furious {

struct ScriptEngine::Impl {
    sol::state lua;
    std::vector<EffectInfo> effects;
    std::vector<std::string> effect_directories;
    std::unordered_map<std::string, sol::protected_function> cached_evaluate_functions;
    Project* project = nullptr;
    TimelineData* timeline_data = nullptr;
    std::string last_error;
    bool initialized = false;

    bool load_effect_script(const std::string& path) {
        try {
            sol::protected_function_result result = lua.safe_script_file(path);
            if (!result.valid()) {
                sol::error err = result;
                last_error = err.what();
                return false;
            }

            sol::table effect_table = lua["effect"];
            if (!effect_table.valid()) {
                last_error = "Script missing 'effect' table: " + path;
                return false;
            }

            EffectInfo info;
            info.id = effect_table["id"].get_or<std::string>("");
            info.name = effect_table["name"].get_or<std::string>("");
            info.script_path = path;

            if (info.id.empty()) {
                last_error = "Effect missing 'id' field: " + path;
                return false;
            }

            sol::protected_function evaluate = lua["evaluate"];
            if (evaluate.valid()) {
                cached_evaluate_functions[info.id] = evaluate;
            }

            sol::optional<sol::table> params_table = effect_table["parameters"];
            if (params_table) {
                for (auto& kv : *params_table) {
                    sol::table param = kv.second.as<sol::table>();
                    EffectParameter ep;
                    ep.name = param["name"].get_or<std::string>("");
                    ep.type = param["type"].get_or<std::string>("string");
                    ep.default_value = param["default"].get_or<std::string>("");

                    sol::optional<sol::table> values = param["values"];
                    if (values) {
                        for (auto& v : *values) {
                            ep.enum_values.push_back(v.second.as<std::string>());
                        }
                    }

                    if (!ep.name.empty()) {
                        info.parameters.push_back(std::move(ep));
                    }
                }
            }

            effects.push_back(std::move(info));
            return true;
        } catch (const std::exception& e) {
            last_error = std::string("Failed to load effect: ") + e.what();
            return false;
        }
    }
};

ScriptEngine::ScriptEngine() : impl_(std::make_unique<Impl>()) {}

ScriptEngine::~ScriptEngine() {
    shutdown();
}

bool ScriptEngine::initialize() {
    if (impl_->initialized) {
        return true;
    }

    try {
        impl_->lua.open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table
        );

        register_lua_bindings(impl_->lua.lua_state());

        impl_->initialized = true;
        return true;
    } catch (const std::exception& e) {
        impl_->last_error = e.what();
        return false;
    }
}

void ScriptEngine::shutdown() {
    impl_->effects.clear();
    impl_->initialized = false;
}

void ScriptEngine::add_effect_directory(const std::string& path) {
    impl_->effect_directories.push_back(path);
}

void ScriptEngine::scan_effect_directories() {
    impl_->effects.clear();
    impl_->cached_evaluate_functions.clear();

    for (const auto& dir : impl_->effect_directories) {
        if (!std::filesystem::exists(dir)) {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".lua") {
                impl_->load_effect_script(entry.path().string());
            }
        }
    }
}

const std::vector<EffectInfo>& ScriptEngine::available_effects() const {
    return impl_->effects;
}

const EffectInfo* ScriptEngine::find_effect(const std::string& effect_id) const {
    for (const auto& effect : impl_->effects) {
        if (effect.id == effect_id) {
            return &effect;
        }
    }
    return nullptr;
}

void ScriptEngine::set_project(Project* project) {
    impl_->project = project;
    bind_project(impl_->lua.lua_state(), project);
}

void ScriptEngine::set_timeline_data(TimelineData* data) {
    impl_->timeline_data = data;
    bind_timeline_data(impl_->lua.lua_state(), data);
}

EffectResult ScriptEngine::evaluate_effect(
    const ClipEffect& effect,
    const EffectContext& context
) {
    EffectResult result;

    const EffectInfo* info = find_effect(effect.effect_id);
    if (!info) {
        impl_->last_error = "Effect not found: " + effect.effect_id;
        return result;
    }

    try {
        auto cached_it = impl_->cached_evaluate_functions.find(effect.effect_id);
        if (cached_it == impl_->cached_evaluate_functions.end()) {
            impl_->last_error = "Effect missing cached 'evaluate' function: " + effect.effect_id;
            return result;
        }
        sol::protected_function evaluate = cached_it->second;

        sol::table lua_context = impl_->lua.create_table();
        lua_context["current_beats"] = context.current_beats;
        lua_context["clip_local_beats"] = context.clip_local_beats;

        if (context.clip) {
            // clip table contains accumulated values from previous effects
            sol::table clip_table = impl_->lua.create_table();
            clip_table["id"] = context.clip->id;
            clip_table["source_id"] = context.clip->source_id;
            clip_table["track_index"] = context.clip->track_index;
            clip_table["start_beat"] = context.clip->start_beat;
            clip_table["duration_beats"] = context.clip->duration_beats;
            clip_table["source_start_seconds"] = context.clip->source_start_seconds;
            clip_table["position_x"] = context.accumulated.position_x;
            clip_table["position_y"] = context.accumulated.position_y;
            clip_table["scale_x"] = context.accumulated.scale_x;
            clip_table["scale_y"] = context.accumulated.scale_y;
            clip_table["rotation"] = context.accumulated.rotation;
            lua_context["clip"] = clip_table;

            // base table contains the clip's original transform values
            sol::table base_table = impl_->lua.create_table();
            base_table["position_x"] = context.clip->position_x;
            base_table["position_y"] = context.clip->position_y;
            base_table["scale_x"] = context.clip->scale_x;
            base_table["scale_y"] = context.clip->scale_y;
            base_table["rotation"] = context.clip->rotation;
            lua_context["base"] = base_table;
        }

        if (context.tempo) {
            sol::table tempo_table = impl_->lua.create_table();
            tempo_table["bpm"] = context.tempo->bpm();
            tempo_table["beat_duration_seconds"] = context.tempo->beat_duration_seconds();

            tempo_table["beats_to_time"] = [tempo = context.tempo](double beats) {
                return tempo->beats_to_time(beats);
            };
            tempo_table["time_to_beats"] = [tempo = context.tempo](double seconds) {
                return tempo->time_to_beats(seconds);
            };

            lua_context["tempo"] = tempo_table;
        }

        sol::table lua_params = impl_->lua.create_table();
        for (const auto& [key, value] : effect.parameters) {
            lua_params[key] = value;
        }

        sol::protected_function_result eval_result = evaluate(lua_context, lua_params);
        if (!eval_result.valid()) {
            sol::error err = eval_result;
            impl_->last_error = err.what();
            return result;
        }

        sol::table result_table = eval_result;
        if (result_table.valid()) {
            result.source_position_seconds = result_table["source_position_seconds"].get_or(0.0);
            result.use_looped_frame = result_table["use_looped_frame"].get_or(false);
            result.loop_start_seconds = result_table["loop_start_seconds"].get_or(0.0);
            result.loop_duration_seconds = result_table["loop_duration_seconds"].get_or(0.0);
            result.position_in_loop_seconds = result_table["position_in_loop_seconds"].get_or(0.0);

            if (result_table["position_x"].valid()) {
                result.position_x = result_table["position_x"].get<float>();
            }
            if (result_table["position_y"].valid()) {
                result.position_y = result_table["position_y"].get<float>();
            }
            if (result_table["scale_x"].valid()) {
                result.scale_x = result_table["scale_x"].get<float>();
            }
            if (result_table["scale_y"].valid()) {
                result.scale_y = result_table["scale_y"].get<float>();
            }
            if (result_table["rotation"].valid()) {
                result.rotation = result_table["rotation"].get<float>();
            }
        }

        return result;
    } catch (const std::exception& e) {
        impl_->last_error = e.what();
        return result;
    }
}

EffectResult ScriptEngine::evaluate_effects(
    const std::vector<ClipEffect>& effects,
    const EffectContext& context
) {
    EffectResult combined;

    EffectContext running_context = context;
    if (context.clip) {
        running_context.accumulated.position_x = context.clip->position_x;
        running_context.accumulated.position_y = context.clip->position_y;
        running_context.accumulated.scale_x = context.clip->scale_x;
        running_context.accumulated.scale_y = context.clip->scale_y;
        running_context.accumulated.rotation = context.clip->rotation;
    }

    for (const auto& effect : effects) {
        if (!effect.enabled) {
            continue;
        }

        EffectResult effect_result = evaluate_effect(effect, running_context);

        if (effect_result.use_looped_frame) {
            combined.use_looped_frame = true;
            combined.loop_start_seconds = effect_result.loop_start_seconds;
            combined.loop_duration_seconds = effect_result.loop_duration_seconds;
            combined.position_in_loop_seconds = effect_result.position_in_loop_seconds;
        }

        if (effect_result.position_x) {
            running_context.accumulated.position_x = *effect_result.position_x;
            combined.position_x = effect_result.position_x;
        }
        if (effect_result.position_y) {
            running_context.accumulated.position_y = *effect_result.position_y;
            combined.position_y = effect_result.position_y;
        }
        if (effect_result.scale_x) {
            running_context.accumulated.scale_x = *effect_result.scale_x;
            combined.scale_x = effect_result.scale_x;
        }
        if (effect_result.scale_y) {
            running_context.accumulated.scale_y = *effect_result.scale_y;
            combined.scale_y = effect_result.scale_y;
        }
        if (effect_result.rotation) {
            running_context.accumulated.rotation = *effect_result.rotation;
            combined.rotation = effect_result.rotation;
        }
    }

    return combined;
}

bool ScriptEngine::is_initialized() const {
    return impl_->initialized;
}

const std::string& ScriptEngine::last_error() const {
    return impl_->last_error;
}

} // namespace furious
