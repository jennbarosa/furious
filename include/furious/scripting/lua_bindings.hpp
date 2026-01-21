#pragma once

struct lua_State;

namespace furious {

class Project;
class TimelineData;

void register_lua_bindings(lua_State* L);
void bind_project(lua_State* L, Project* project);
void bind_timeline_data(lua_State* L, TimelineData* data);

} // namespace furious
