#include "furious/core/command.hpp"

namespace furious {

void CommandHistory::execute(std::unique_ptr<Command> cmd) {
    cmd->execute();
    undo_stack_.push_back(std::move(cmd));
    redo_stack_.clear();

    if (undo_stack_.size() > MAX_HISTORY) {
        undo_stack_.erase(undo_stack_.begin());
    }
}

bool CommandHistory::can_undo() const {
    return !undo_stack_.empty();
}

bool CommandHistory::can_redo() const {
    return !redo_stack_.empty();
}

void CommandHistory::undo() {
    if (undo_stack_.empty()) {
        return;
    }

    auto cmd = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    cmd->undo();
    redo_stack_.push_back(std::move(cmd));
}

void CommandHistory::redo() {
    if (redo_stack_.empty()) {
        return;
    }

    auto cmd = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    cmd->execute();
    undo_stack_.push_back(std::move(cmd));
}

void CommandHistory::clear() {
    undo_stack_.clear();
    redo_stack_.clear();
}

std::string CommandHistory::undo_description() const {
    return undo_stack_.empty() ? "" : undo_stack_.back()->description();
}

std::string CommandHistory::redo_description() const {
    return redo_stack_.empty() ? "" : redo_stack_.back()->description();
}

} // namespace furious
