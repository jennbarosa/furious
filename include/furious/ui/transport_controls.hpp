#pragma once

#include "furious/core/project.hpp"

namespace furious {

class TransportControls {
public:
    explicit TransportControls(Project& project);

    void render();

    [[nodiscard]] bool is_playing() const;
    void set_playing(bool playing);

    [[nodiscard]] bool metronome_enabled() const;
    void set_metronome_enabled(bool enabled);

    [[nodiscard]] bool follow_playhead() const;
    void set_follow_playhead(bool follow);

    [[nodiscard]] bool loop_enabled() const;
    void set_loop_enabled(bool enabled);

    [[nodiscard]] bool reset_requested();
    [[nodiscard]] bool save_requested();
    [[nodiscard]] bool load_requested();
    [[nodiscard]] const std::string& requested_filepath() const { return requested_filepath_; }

    void set_current_project_path(const std::string& path) { current_project_path_ = path; }

private:
    Project& project_;
    bool is_playing_ = false;
    bool metronome_enabled_ = false;
    bool follow_playhead_ = true;
    bool loop_enabled_ = false;
    bool reset_requested_ = false;
    bool save_requested_ = false;
    bool load_requested_ = false;
    std::string requested_filepath_;
    std::string current_project_path_;
};

} // namespace furious
