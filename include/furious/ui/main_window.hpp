#pragma once

#include "furious/core/project.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/command.hpp"
#include "furious/core/pattern_library.hpp"
#include "furious/core/pattern_evaluator.hpp"
#include "furious/core/pitch_library.hpp"
#include "furious/ui/viewport.hpp"
#include "furious/ui/timeline.hpp"
#include "furious/ui/transport_controls.hpp"
#include "furious/ui/profiler_window.hpp"
#include "furious/ui/patterns_window.hpp"
#include "furious/ui/pitch_editor_window.hpp"
#include "furious/audio/audio_engine.hpp"
#include "furious/video/video_engine.hpp"
#include "furious/video/source_library.hpp"
#include "furious/scripting/script_engine.hpp"
#include <memory>
#include <unordered_map>

struct GLFWwindow;

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
    [[nodiscard]] PatternLibrary& pattern_library();
    [[nodiscard]] PitchLibrary& pitch_library();
    [[nodiscard]] TimelineData& timeline_data();
    [[nodiscard]] Timeline& timeline();
    [[nodiscard]] TransportControls& transport_controls();
    [[nodiscard]] ScriptEngine& script_engine();
    [[nodiscard]] bool is_playing() const;

    bool load_audio_file(const std::string& filepath);
    std::string import_source(const std::string& filepath);

    bool save_project(const std::string& filepath);
    bool load_project(const std::string& filepath);
    [[nodiscard]] const std::string& current_project_path() const { return current_project_path_; }

    [[nodiscard]] bool needs_continuous_rendering() const;

    [[nodiscard]] std::string window_title() const;
    [[nodiscard]] bool is_dirty() const { return dirty_; }
    void mark_dirty();

    void execute_command(std::unique_ptr<Command> cmd);
    [[nodiscard]] CommandHistory& command_history() { return command_history_; }

    void set_glfw_window(GLFWwindow* window) { glfw_window_ = window; }

private:
    GLFWwindow* glfw_window_ = nullptr;
    Project project_;
    TimelineData timeline_data_;
    SourceLibrary source_library_;
    PatternLibrary pattern_library_;
    PitchLibrary pitch_library_;
    Viewport viewport_;
    Timeline timeline_;
    TransportControls transport_controls_;
    AudioEngine audio_engine_;
    VideoEngine video_engine_;
    ScriptEngine script_engine_;
    ProfilerWindow profiler_;
    PatternsWindow patterns_window_;
    PitchEditorWindow pitch_editor_;
    PatternEvaluator pattern_evaluator_;
    CommandHistory command_history_;

    bool first_frame_ = true;
    bool layout_loaded_ = false;
    double last_playhead_beats_ = 0.0;
    double last_synced_beats_ = -1.0;
    bool video_synced_once_ = false;
    std::string current_project_path_;
    std::string pending_video_error_;
    bool dirty_ = false;
    std::string pending_source_removal_;

    bool cache_building_ = false;
    size_t cache_current_clip_ = 0;
    size_t cache_total_clips_ = 0;

    enum class EditMode { None, Transform, Effect };
    EditMode edit_mode_ = EditMode::None;
    TimelineClip property_edit_initial_state_;

    double preview_end_time_ = 0.0;

    std::unordered_map<std::string, float> last_pitch_shift_cents_;

    struct PitchDetectionCache {
        int64_t last_source_frame = -99999;
        float detected_frequency = 0.0f;
        float confidence = 0.0f;
        int frames_since_detection = 999;
    };
    std::unordered_map<std::string, PitchDetectionCache> pitch_detection_cache_;

    void setup_dockspace();
    void build_default_layout(unsigned int dockspace_id);
    void render_audio_panel();
    void render_sources_panel();
    void render_effects_panel();
    void render_loading_modal();
    void sync_video_to_playhead();
    void sync_audio_to_playhead();
    void cache_all_clips();
    void start_cache_building();
    bool cache_next_clip();
    void handle_keyboard_shortcuts();
    double compute_pitch_editor_playhead() const;
    void preview_pitch_at_subdivision(int subdivision, int midi_note, float duration, float bgm_vol, float clip_vol);
};

} // namespace furious
