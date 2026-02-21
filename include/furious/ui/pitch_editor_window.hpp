#pragma once

#include "furious/core/command.hpp"
#include "furious/core/pattern_library.hpp"
#include "furious/core/pitch_library.hpp"
#include "furious/core/pitch_track.hpp"
#include "furious/audio/audio_clip.hpp"
#include "furious/core/tempo.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace furious {

class PitchEditorWindow {
public:
    PitchEditorWindow() = default;

    void render();

    void set_pitch_library(PitchLibrary* library) { library_ = library; }
    void set_pattern_library(PatternLibrary* library) { pattern_library_ = library; }
    void set_audio_clip(const AudioClip* clip) { audio_clip_ = clip; }
    void set_tempo(const Tempo* tempo) { tempo_ = tempo; }
    void set_command_callback(std::function<void(std::unique_ptr<Command>)> callback) {
        execute_command_ = std::move(callback);
    }
    void set_preview_callback(std::function<void(int subdivision, int midi_note, float duration, float bgm_vol, float clip_vol)> callback) {
        preview_callback_ = std::move(callback);
    }

    [[nodiscard]] bool is_open() const { return open_; }
    void set_open(bool open) { open_ = open; }

    [[nodiscard]] const std::string& selected_track_id() const { return selected_track_id_; }

    [[nodiscard]] bool preview_enabled() const { return preview_enabled_; }
    void set_preview_enabled(bool enabled) { preview_enabled_ = enabled; }
    [[nodiscard]] float preview_duration() const { return preview_duration_; }
    void set_preview_duration(float duration) { preview_duration_ = duration; }

    void set_playhead_beat(double beat) { playhead_beat_ = beat; }
    [[nodiscard]] bool consume_play_toggle_request();

    [[nodiscard]] float bgm_volume() const { return bgm_volume_; }
    void set_bgm_volume(float vol) { bgm_volume_ = vol; }
    [[nodiscard]] float clip_volume() const { return clip_volume_; }
    void set_clip_volume(float vol) { clip_volume_ = vol; }

    [[nodiscard]] const std::string& overlay_pattern_id() const { return overlay_pattern_id_; }
    void set_overlay_pattern_id(const std::string& id) { overlay_pattern_id_ = id; }

private:
    PitchLibrary* library_ = nullptr;
    PatternLibrary* pattern_library_ = nullptr;
    const AudioClip* audio_clip_ = nullptr;
    const Tempo* tempo_ = nullptr;
    std::function<void(std::unique_ptr<Command>)> execute_command_;
    std::function<void(int subdivision, int midi_note, float duration, float bgm_vol, float clip_vol)> preview_callback_;

    bool open_ = false;
    std::string selected_track_id_;
    char search_buffer_[256] = {};
    char rename_buffer_[256] = {};
    bool renaming_ = false;

    std::set<int> selected_note_indices_;

    float zoom_ = 1.0f;
    float scroll_offset_ = 0.0f;
    float last_canvas_width_ = 0.0f;
    static constexpr float BASE_PIXELS_PER_SUBDIVISION = 20.0f;
    int subdivisions_per_beat_ = 4;

    static constexpr int MIN_MIDI_NOTE = 36;
    static constexpr int MAX_MIDI_NOTE = 84;
    static constexpr int NOTE_RANGE = MAX_MIDI_NOTE - MIN_MIDI_NOTE;
    static constexpr float ROW_HEIGHT = 16.0f;

    std::optional<PitchTrack> edit_initial_state_;
    bool editing_ = false;

    bool box_selecting_ = false;
    float box_select_start_x_ = 0.0f;
    float box_select_start_y_ = 0.0f;
    float box_select_end_x_ = 0.0f;
    float box_select_end_y_ = 0.0f;
    int pending_click_subdivision_ = -1;
    int pending_click_midi_note_ = -1;
    bool pending_had_selection_ = false;

    bool preview_enabled_ = false;
    float preview_duration_ = 0.5f;
    float vertical_zoom_ = 1.0f;

    double playhead_beat_ = 0.0;
    bool play_toggle_requested_ = false;

    float bgm_volume_ = 1.0f;
    float clip_volume_ = 1.0f;
    std::string overlay_pattern_id_;

    void clamp_scroll(const PitchTrack& track);
    void trigger_preview(const PitchTrack& track, int subdivision);

    void render_track_list();
    void render_track_editor();
    void render_grid(PitchTrack& track);
    void render_note_properties(PitchTrack& track);

    void estimate_from_bgm(PitchTrack& track);

    void begin_edit(const PitchTrack& track);
    void end_edit(PitchTrack& track);

    void handle_keyboard_shortcuts(PitchTrack& track);
    void select_all_notes(const PitchTrack& track);
    void shift_selected_notes(PitchTrack& track, int semitones);

    [[nodiscard]] float midi_to_y(int midi_note, float canvas_y, float canvas_height) const;
    [[nodiscard]] int y_to_midi(float y, float canvas_y, float canvas_height) const;
};

} // namespace furious
