#pragma once

#include <memory>
#include <string>
#include <vector>

namespace furious {

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    [[nodiscard]] virtual std::string description() const = 0;
};

class CommandHistory {
public:
    void execute(std::unique_ptr<Command> cmd);

    [[nodiscard]] bool can_undo() const;
    [[nodiscard]] bool can_redo() const;

    void undo();
    void redo();

    void clear();

    [[nodiscard]] std::string undo_description() const;
    [[nodiscard]] std::string redo_description() const;

private:
    std::vector<std::unique_ptr<Command>> undo_stack_;
    std::vector<std::unique_ptr<Command>> redo_stack_;

    static constexpr size_t MAX_HISTORY = 100;
};

} // namespace furious
