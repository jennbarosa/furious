#pragma once

#include "furious/core/tempo.hpp"
#include <string>

namespace furious {

class Project {
public:
    Project();
    explicit Project(std::string name);

    [[nodiscard]] const std::string& name() const;
    void set_name(std::string name);

    [[nodiscard]] Tempo& tempo();
    [[nodiscard]] const Tempo& tempo() const;

    [[nodiscard]] NoteSubdivision grid_subdivision() const;
    void set_grid_subdivision(NoteSubdivision subdivision);

    [[nodiscard]] double fps() const { return fps_; }
    void set_fps(double fps) { fps_ = fps; }

    [[nodiscard]] bool snap_enabled() const { return snap_enabled_; }
    void set_snap_enabled(bool enabled) { snap_enabled_ = enabled; }

private:
    std::string name_;
    Tempo tempo_;
    NoteSubdivision grid_subdivision_ = NoteSubdivision::Quarter;
    double fps_ = 30.0;
    bool snap_enabled_ = true;
};

} // namespace furious
