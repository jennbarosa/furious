#pragma once

#include "furious/core/command.hpp"
#include "furious/core/pattern.hpp"
#include "furious/core/pattern_library.hpp"
#include "furious/core/timeline_data.hpp"
#include <utility>

namespace furious {

class AddPatternCommand : public Command {
public:
    AddPatternCommand(PatternLibrary& library, Pattern pattern)
        : library_(library), pattern_(std::move(pattern)) {}

    void execute() override {
        library_.add_pattern(pattern_);
    }

    void undo() override {
        library_.remove_pattern(pattern_.id);
    }

    [[nodiscard]] std::string description() const override {
        return "Add pattern";
    }

private:
    PatternLibrary& library_;
    Pattern pattern_;
};

class RemovePatternCommand : public Command {
public:
    RemovePatternCommand(PatternLibrary& library, std::string pattern_id)
        : library_(library), pattern_id_(std::move(pattern_id)) {}

    void execute() override {
        if (const Pattern* pattern = library_.find_pattern(pattern_id_)) {
            saved_pattern_ = *pattern;
        }
        library_.remove_pattern(pattern_id_);
    }

    void undo() override {
        library_.add_pattern(saved_pattern_);
    }

    [[nodiscard]] std::string description() const override {
        return "Remove pattern";
    }

private:
    PatternLibrary& library_;
    std::string pattern_id_;
    Pattern saved_pattern_;
};

class ModifyPatternCommand : public Command {
public:
    ModifyPatternCommand(PatternLibrary& library, std::string pattern_id,
                         Pattern old_state, Pattern new_state,
                         std::string action_name = "Modify pattern")
        : library_(library)
        , pattern_id_(std::move(pattern_id))
        , old_state_(std::move(old_state))
        , new_state_(std::move(new_state))
        , action_name_(std::move(action_name)) {}

    void execute() override {
        if (Pattern* pattern = library_.find_pattern(pattern_id_)) {
            *pattern = new_state_;
        }
    }

    void undo() override {
        if (Pattern* pattern = library_.find_pattern(pattern_id_)) {
            *pattern = old_state_;
        }
    }

    [[nodiscard]] std::string description() const override {
        return action_name_;
    }

private:
    PatternLibrary& library_;
    std::string pattern_id_;
    Pattern old_state_;
    Pattern new_state_;
    std::string action_name_;
};

class ToggleClipPatternCommand : public Command {
public:
    ToggleClipPatternCommand(TimelineData& data, std::string clip_id,
                              std::string pattern_id, bool add)
        : data_(data)
        , clip_id_(std::move(clip_id))
        , pattern_id_(std::move(pattern_id))
        , add_(add) {}

    void execute() override {
        if (TimelineClip* clip = data_.find_clip(clip_id_)) {
            if (add_) {
                ClipPatternReference ref;
                ref.pattern_id = pattern_id_;
                clip->patterns.push_back(ref);
            } else {
                std::erase_if(clip->patterns, [this](const ClipPatternReference& r) {
                    return r.pattern_id == pattern_id_;
                });
            }
        }
    }

    void undo() override {
        if (TimelineClip* clip = data_.find_clip(clip_id_)) {
            if (add_) {
                std::erase_if(clip->patterns, [this](const ClipPatternReference& r) {
                    return r.pattern_id == pattern_id_;
                });
            } else {
                ClipPatternReference ref;
                ref.pattern_id = pattern_id_;
                clip->patterns.push_back(ref);
            }
        }
    }

    [[nodiscard]] std::string description() const override {
        return add_ ? "Add pattern to clip" : "Remove pattern from clip";
    }

private:
    TimelineData& data_;
    std::string clip_id_;
    std::string pattern_id_;
    bool add_;
};

} // namespace furious
