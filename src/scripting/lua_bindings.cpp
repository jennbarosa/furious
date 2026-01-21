#include "furious/scripting/lua_bindings.hpp"
#include "furious/core/project.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/tempo.hpp"
#include "furious/core/timeline_clip.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace furious {

void register_lua_bindings(lua_State* L) {
    sol::state_view lua(L);

    lua.new_usertype<Tempo>("Tempo",
        sol::no_constructor,
        "bpm", &Tempo::bpm,
        "beat_duration_seconds", &Tempo::beat_duration_seconds,
        "beats_to_time", &Tempo::beats_to_time,
        "time_to_beats", &Tempo::time_to_beats
    );

    lua["furious"] = lua.create_table();

    lua["furious"]["period_to_beats"] = [](const std::string& period) -> double {
        if (period == "1/16" || period == "sixteenth") return 0.25;
        if (period == "1/8" || period == "eighth") return 0.5;
        if (period == "1/4" || period == "quarter") return 1.0;
        if (period == "1/2" || period == "half") return 2.0;
        if (period == "measure") return 4.0;
        return 1.0;
    };
}

void bind_project(lua_State* L, Project* project) {
    if (!project) return;

    sol::state_view lua(L);
    sol::table furious = lua["furious"];

    furious["project"] = lua.create_table_with(
        "bpm", [project]() { return project->tempo().bpm(); },
        "set_bpm", [project](double bpm) { project->tempo().set_bpm(bpm); }
    );
}

void bind_timeline_data(lua_State* L, TimelineData* data) {
    if (!data) return;

    sol::state_view lua(L);
    sol::table furious = lua["furious"];

    furious["timeline"] = lua.create_table_with(
        "track_count", [data]() { return data->tracks().size(); },
        "clip_count", [data]() { return data->clips().size(); }
    );
}

} // namespace furious
