#include "furious/core/project_data.hpp"
#include "furious/core/enum_utils.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace furious {

namespace {

nlohmann::json source_to_json(const MediaSource& source) {
    nlohmann::json j;
    j["id"] = source.id;
    j["filepath"] = source.filepath;
    j["name"] = source.name;
    j["type"] = enum_to_string(source.type);
    j["duration_seconds"] = source.duration_seconds;
    j["width"] = source.width;
    j["height"] = source.height;
    j["fps"] = source.fps;
    return j;
}

MediaSource json_to_source(const nlohmann::json& j) {
    MediaSource source;
    source.id = j.value("id", "");
    source.filepath = j.value("filepath", "");
    source.name = j.value("name", "");
    source.type = string_to_enum(j.value("type", "video"), MediaType::Video);
    source.duration_seconds = j.value("duration_seconds", 0.0);
    source.width = j.value("width", 0);
    source.height = j.value("height", 0);
    source.fps = j.value("fps", 30.0);
    return source;
}

nlohmann::json track_to_json(const Track& track) {
    nlohmann::json j;
    j["name"] = track.name;
    return j;
}

Track json_to_track(const nlohmann::json& j) {
    Track track;
    track.name = j.value("name", "");
    return track;
}

nlohmann::json effect_to_json(const ClipEffect& effect) {
    nlohmann::json j;
    j["effect_id"] = effect.effect_id;
    j["enabled"] = effect.enabled;
    j["parameters"] = nlohmann::json::object();
    for (const auto& [key, value] : effect.parameters) {
        j["parameters"][key] = value;
    }
    return j;
}

ClipEffect json_to_effect(const nlohmann::json& j) {
    ClipEffect effect;
    effect.effect_id = j.value("effect_id", "");
    effect.enabled = j.value("enabled", true);
    if (j.contains("parameters") && j["parameters"].is_object()) {
        for (const auto& [key, value] : j["parameters"].items()) {
            effect.parameters[key] = value.get<std::string>();
        }
    }
    return effect;
}

std::string property_to_string(PatternTargetProperty prop) {
    switch (prop) {
        case PatternTargetProperty::PositionX: return "position_x";
        case PatternTargetProperty::PositionY: return "position_y";
        case PatternTargetProperty::ScaleX: return "scale_x";
        case PatternTargetProperty::ScaleY: return "scale_y";
        case PatternTargetProperty::Rotation: return "rotation";
        case PatternTargetProperty::FlipH: return "flip_h";
        case PatternTargetProperty::FlipV: return "flip_v";
    }
    return "scale_x";
}

PatternTargetProperty string_to_property(const std::string& str) {
    if (str == "position_x") return PatternTargetProperty::PositionX;
    if (str == "position_y") return PatternTargetProperty::PositionY;
    if (str == "scale_x") return PatternTargetProperty::ScaleX;
    if (str == "scale_y") return PatternTargetProperty::ScaleY;
    if (str == "rotation") return PatternTargetProperty::Rotation;
    if (str == "flip_h") return PatternTargetProperty::FlipH;
    if (str == "flip_v") return PatternTargetProperty::FlipV;
    return PatternTargetProperty::ScaleX;
}

nlohmann::json trigger_to_json(const PatternTrigger& trigger) {
    nlohmann::json j;
    j["subdivision_index"] = trigger.subdivision_index;
    j["target_property"] = property_to_string(trigger.target);
    j["value"] = trigger.value;
    return j;
}

PatternTrigger json_to_trigger(const nlohmann::json& j) {
    PatternTrigger trigger;
    trigger.subdivision_index = j.value("subdivision_index", 0);
    trigger.target = string_to_property(j.value("target_property", "scale_x"));
    trigger.value = j.value("value", 1.0f);
    return trigger;
}

nlohmann::json settings_to_json(const PatternPropertySettings& settings) {
    nlohmann::json j;
    j["restart_on_trigger"] = settings.restart_on_trigger;
    return j;
}

PatternPropertySettings json_to_settings(const nlohmann::json& j) {
    PatternPropertySettings settings;
    settings.restart_on_trigger = j.value("restart_on_trigger", false);
    return settings;
}

nlohmann::json pattern_to_json(const Pattern& pattern) {
    nlohmann::json j;
    j["id"] = pattern.id;
    j["name"] = pattern.name;
    j["length_subdivisions"] = pattern.length_subdivisions;
    j["triggers"] = nlohmann::json::array();
    for (const auto& trigger : pattern.triggers) {
        j["triggers"].push_back(trigger_to_json(trigger));
    }
    j["position_x_settings"] = settings_to_json(pattern.position_x_settings);
    j["position_y_settings"] = settings_to_json(pattern.position_y_settings);
    j["scale_x_settings"] = settings_to_json(pattern.scale_x_settings);
    j["scale_y_settings"] = settings_to_json(pattern.scale_y_settings);
    j["rotation_settings"] = settings_to_json(pattern.rotation_settings);
    j["flip_h_settings"] = settings_to_json(pattern.flip_h_settings);
    j["flip_v_settings"] = settings_to_json(pattern.flip_v_settings);
    return j;
}

Pattern json_to_pattern(const nlohmann::json& j) {
    Pattern pattern;
    pattern.id = j.value("id", "");
    pattern.name = j.value("name", "Pattern");
    pattern.length_subdivisions = j.value("length_subdivisions", 16);
    if (j.contains("triggers") && j["triggers"].is_array()) {
        for (const auto& t : j["triggers"]) {
            pattern.triggers.push_back(json_to_trigger(t));
        }
    }
    if (j.contains("position_x_settings")) {
        pattern.position_x_settings = json_to_settings(j["position_x_settings"]);
    }
    if (j.contains("position_y_settings")) {
        pattern.position_y_settings = json_to_settings(j["position_y_settings"]);
    }
    if (j.contains("scale_x_settings")) {
        pattern.scale_x_settings = json_to_settings(j["scale_x_settings"]);
    }
    if (j.contains("scale_y_settings")) {
        pattern.scale_y_settings = json_to_settings(j["scale_y_settings"]);
    }
    if (j.contains("rotation_settings")) {
        pattern.rotation_settings = json_to_settings(j["rotation_settings"]);
    }
    if (j.contains("flip_h_settings")) {
        pattern.flip_h_settings = json_to_settings(j["flip_h_settings"]);
    }
    if (j.contains("flip_v_settings")) {
        pattern.flip_v_settings = json_to_settings(j["flip_v_settings"]);
    }
    return pattern;
}

nlohmann::json pattern_ref_to_json(const ClipPatternReference& ref) {
    nlohmann::json j;
    j["pattern_id"] = ref.pattern_id;
    j["enabled"] = ref.enabled;
    j["offset_subdivisions"] = ref.offset_subdivisions;
    return j;
}

ClipPatternReference json_to_pattern_ref(const nlohmann::json& j) {
    ClipPatternReference ref;
    ref.pattern_id = j.value("pattern_id", "");
    ref.enabled = j.value("enabled", true);
    ref.offset_subdivisions = j.value("offset_subdivisions", 0);
    return ref;
}

nlohmann::json clip_to_json(const TimelineClip& clip) {
    nlohmann::json j;
    j["id"] = clip.id;
    j["source_id"] = clip.source_id;
    j["track_index"] = clip.track_index;
    j["start_beat"] = clip.start_beat;
    j["duration_beats"] = clip.duration_beats;
    j["source_start_seconds"] = clip.source_start_seconds;
    j["position_x"] = clip.position_x;
    j["position_y"] = clip.position_y;
    j["scale_x"] = clip.scale_x;
    j["scale_y"] = clip.scale_y;
    j["rotation"] = clip.rotation;

    if (!clip.effects.empty()) {
        j["effects"] = nlohmann::json::array();
        for (const auto& effect : clip.effects) {
            j["effects"].push_back(effect_to_json(effect));
        }
    }

    if (!clip.patterns.empty()) {
        j["patterns"] = nlohmann::json::array();
        for (const auto& ref : clip.patterns) {
            j["patterns"].push_back(pattern_ref_to_json(ref));
        }
    }

    return j;
}

TimelineClip json_to_clip(const nlohmann::json& j) {
    TimelineClip clip;
    clip.id = j.value("id", "");
    clip.source_id = j.value("source_id", "");
    clip.track_index = j.value("track_index", 0);
    clip.start_beat = j.value("start_beat", 0.0);
    clip.duration_beats = j.value("duration_beats", 4.0);
    clip.source_start_seconds = j.value("source_start_seconds", 0.0);
    clip.position_x = j.value("position_x", j.value("viewport_x", 0.0f));
    clip.position_y = j.value("position_y", j.value("viewport_y", 0.0f));
    clip.scale_x = j.value("scale_x", j.value("viewport_scale", 1.0f));
    clip.scale_y = j.value("scale_y", j.value("viewport_scale", 1.0f));
    clip.rotation = j.value("rotation", 0.0f);

    if (j.contains("effects") && j["effects"].is_array()) {
        for (const auto& effect_json : j["effects"]) {
            clip.effects.push_back(json_to_effect(effect_json));
        }
    }

    if (j.contains("patterns") && j["patterns"].is_array()) {
        for (const auto& ref_json : j["patterns"]) {
            clip.patterns.push_back(json_to_pattern_ref(ref_json));
        }
    }

    return clip;
}

} // namespace

bool ProjectData::save_to_file(const std::string& filepath) const {
    nlohmann::json j;

    j["version"] = 2;
    j["name"] = name;
    j["tempo"]["bpm"] = bpm;
    j["tempo"]["grid_subdivision"] = enum_to_string(grid_subdivision);
    j["tempo"]["fps"] = fps;
    j["transport"]["metronome_enabled"] = metronome_enabled;
    j["transport"]["follow_playhead"] = follow_playhead;
    j["transport"]["loop_enabled"] = loop_enabled;
    j["transport"]["playhead_beat"] = playhead_beat;
    j["timeline"]["zoom"] = timeline_zoom;
    j["timeline"]["zoom_y"] = timeline_zoom_y;
    j["timeline"]["scroll"] = timeline_scroll;
    j["timeline"]["scroll_y"] = timeline_scroll_y;
    j["window"]["width"] = window_width;
    j["window"]["height"] = window_height;
    if (!imgui_layout.empty()) {
        j["window"]["imgui_layout"] = imgui_layout;
    }
    j["audio"]["filepath"] = audio_filepath;
    j["audio"]["clip_start_seconds"] = clip_start_seconds;
    j["audio"]["clip_end_seconds"] = clip_end_seconds;

    j["sources"] = nlohmann::json::array();
    for (const auto& source : sources) {
        j["sources"].push_back(source_to_json(source));
    }

    j["tracks"] = nlohmann::json::array();
    for (const auto& track : tracks) {
        j["tracks"].push_back(track_to_json(track));
    }

    j["clips"] = nlohmann::json::array();
    for (const auto& clip : clips) {
        j["clips"].push_back(clip_to_json(clip));
    }

    j["patterns"] = nlohmann::json::array();
    for (const auto& pattern : patterns) {
        j["patterns"].push_back(pattern_to_json(pattern));
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << j.dump(2);
    return file.good();
}

bool ProjectData::load_from_file(const std::string& filepath, ProjectData& out_data) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }

    int version = j.value("version", 0);
    if (version < 1) {
        return false;
    }

    out_data.name = j.value("name", "Untitled Project");

    if (j.contains("tempo")) {
        auto& tempo = j["tempo"];
        out_data.bpm = tempo.value("bpm", 120.0);
        out_data.grid_subdivision = string_to_enum(tempo.value("grid_subdivision", "quarter"), NoteSubdivision::Quarter);
        out_data.fps = tempo.value("fps", 30.0);
    }

    if (j.contains("transport")) {
        auto& transport = j["transport"];
        out_data.metronome_enabled = transport.value("metronome_enabled", false);
        out_data.follow_playhead = transport.value("follow_playhead", true);
        out_data.loop_enabled = transport.value("loop_enabled", false);
        out_data.playhead_beat = transport.value("playhead_beat", 0.0);
    }

    if (j.contains("timeline")) {
        auto& timeline = j["timeline"];
        out_data.timeline_zoom = timeline.value("zoom", 1.0f);
        out_data.timeline_zoom_y = timeline.value("zoom_y", 1.0f);
        out_data.timeline_scroll = timeline.value("scroll", 0.0f);
        out_data.timeline_scroll_y = timeline.value("scroll_y", 0.0f);
    }

    if (j.contains("window")) {
        auto& window = j["window"];
        out_data.window_width = window.value("width", 1280);
        out_data.window_height = window.value("height", 720);
        out_data.imgui_layout = window.value("imgui_layout", "");
    }

    if (j.contains("audio")) {
        auto& audio = j["audio"];
        out_data.audio_filepath = audio.value("filepath", "");
        out_data.clip_start_seconds = audio.value("clip_start_seconds", 0.0);
        out_data.clip_end_seconds = audio.value("clip_end_seconds", 0.0);
    }

    out_data.sources.clear();
    out_data.tracks.clear();
    out_data.clips.clear();
    out_data.patterns.clear();

    if (j.contains("sources") && j["sources"].is_array()) {
        for (const auto& src_json : j["sources"]) {
            out_data.sources.push_back(json_to_source(src_json));
        }
    }

    if (j.contains("tracks") && j["tracks"].is_array()) {
        for (const auto& track_json : j["tracks"]) {
            out_data.tracks.push_back(json_to_track(track_json));
        }
    }

    if (j.contains("clips") && j["clips"].is_array()) {
        for (const auto& clip_json : j["clips"]) {
            out_data.clips.push_back(json_to_clip(clip_json));
        }
    }

    if (j.contains("patterns") && j["patterns"].is_array()) {
        for (const auto& pattern_json : j["patterns"]) {
            out_data.patterns.push_back(json_to_pattern(pattern_json));
        }
    }

    return true;
}

} // namespace furious
