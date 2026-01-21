#include "furious/core/project.hpp"
#include <utility>

namespace furious {

Project::Project() : Project("Untitled Project") {}

Project::Project(std::string name) : name_(std::move(name)) {}

const std::string& Project::name() const {
    return name_;
}

void Project::set_name(std::string name) {
    name_ = std::move(name);
}

Tempo& Project::tempo() {
    return tempo_;
}

const Tempo& Project::tempo() const {
    return tempo_;
}

NoteSubdivision Project::grid_subdivision() const {
    return grid_subdivision_;
}

void Project::set_grid_subdivision(NoteSubdivision subdivision) {
    grid_subdivision_ = subdivision;
}

} // namespace furious
