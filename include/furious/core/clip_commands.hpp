#pragma once

#include "furious/core/command.hpp"
#include "furious/core/timeline_data.hpp"
#include "furious/core/timeline_clip.hpp"
#include <utility>

namespace furious {

class AddClipCommand : public Command {
public:
    AddClipCommand(TimelineData& data, TimelineClip clip)
        : data_(data), clip_(std::move(clip)) {}

    void execute() override {
        data_.add_clip(clip_);
    }

    void undo() override {
        data_.remove_clip(clip_.id);
    }

    [[nodiscard]] std::string description() const override {
        return "Add clip";
    }

private:
    TimelineData& data_;
    TimelineClip clip_;
};

class RemoveClipCommand : public Command {
public:
    RemoveClipCommand(TimelineData& data, std::string clip_id)
        : data_(data), clip_id_(std::move(clip_id)) {}

    void execute() override {
        if (const TimelineClip* clip = data_.find_clip(clip_id_)) {
            saved_clip_ = *clip;
        }
        data_.remove_clip(clip_id_);
    }

    void undo() override {
        data_.add_clip(saved_clip_);
    }

    [[nodiscard]] std::string description() const override {
        return "Remove clip";
    }

private:
    TimelineData& data_;
    std::string clip_id_;
    TimelineClip saved_clip_;
};

class ModifyClipCommand : public Command {
public:
    ModifyClipCommand(TimelineData& data, std::string clip_id,
                      TimelineClip old_state, TimelineClip new_state,
                      std::string action_name = "Modify clip")
        : data_(data)
        , clip_id_(std::move(clip_id))
        , old_state_(std::move(old_state))
        , new_state_(std::move(new_state))
        , action_name_(std::move(action_name)) {}

    void execute() override {
        if (TimelineClip* clip = data_.find_clip(clip_id_)) {
            *clip = new_state_;
        }
    }

    void undo() override {
        if (TimelineClip* clip = data_.find_clip(clip_id_)) {
            *clip = old_state_;
        }
    }

    [[nodiscard]] std::string description() const override {
        return action_name_;
    }

private:
    TimelineData& data_;
    std::string clip_id_;
    TimelineClip old_state_;
    TimelineClip new_state_;
    std::string action_name_;
};

} // namespace furious
