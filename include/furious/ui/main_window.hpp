#pragma once

#include "furious/core/project.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/ui/viewport.hpp"
#include "furious/ui/timeline.hpp"
#include "furious/ui/transport_controls.hpp"
#include "furious/ui/profiler_window.hpp"
#include "furious/audio/audio_engine.hpp"
#include "furious/video/video_engine.hpp"
#include "furious/video/source_library.hpp"
#include "furious/scripting/script_engine.hpp"
#include <memory>

namespace furious {

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void render();

    [[nodiscard]] Project& project();
    [[nodiscard]] AudioEngine& audio_engine();
    [[nodiscard]] VideoEngine& video_engine();
    [[nodiscard]] SourceLibrary& source_library();
    [[nodiscard]] TimelineData& timeline_data();
    [[nodiscard]] Timeline& timeline();
    [[nodiscard]] TransportControls& transport_controls();
    [[nodiscard]] ScriptEngine& script_engine();

    bool load_audio_file(const std::string& filepath);
    std::string import_source(const std::string& filepath);

    bool save_project(const std::string& filepath);
    bool load_project(const std::string& filepath);
    [[nodiscard]] const std::string& current_project_path() const { return current_project_path_; }

    [[nodiscard]] bool needs_continuous_rendering() const;

    [[nodiscard]] std::string window_title() const;
    [[nodiscard]] bool is_dirty() const { return dirty_; }
    void mark_dirty();

private:
    Project project_;
    TimelineData timeline_data_;
    SourceLibrary source_library_;
    Viewport viewport_;
    Timeline timeline_;
    TransportControls transport_controls_;
    AudioEngine audio_engine_;
    VideoEngine video_engine_;
    ScriptEngine script_engine_;
    ProfilerWindow profiler_;

    bool first_frame_ = true;
    double last_playhead_beats_ = 0.0;
    std::string current_project_path_;
    bool dirty_ = false;
    std::string pending_source_removal_;

    bool cache_building_ = false;
    size_t cache_current_clip_ = 0;
    size_t cache_total_clips_ = 0;

    void setup_dockspace();
    void build_default_layout(unsigned int dockspace_id);
    void render_audio_panel();
    void render_sources_panel();
    void render_effects_panel();
    void render_loading_modal();
    void sync_video_to_playhead();
    void cache_all_clips();
    void start_cache_building();
    bool cache_next_clip();
};

} // namespace furious
