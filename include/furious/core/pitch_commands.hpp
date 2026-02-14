#pragma once

#include "furious/core/command.hpp"
#include "furious/core/pitch_library.hpp"
#include "furious/core/pitch_track.hpp"
#include <utility>

namespace furious {

class AddPitchTrackCommand : public Command {
public:
    AddPitchTrackCommand(PitchLibrary& library, PitchTrack track)
        : library_(library), track_(std::move(track)) {}

    void execute() override {
        library_.add_track(track_);
    }

    void undo() override {
        library_.remove_track(track_.id);
    }

    [[nodiscard]] std::string description() const override {
        return "Add pitch track";
    }

private:
    PitchLibrary& library_;
    PitchTrack track_;
};

class RemovePitchTrackCommand : public Command {
public:
    RemovePitchTrackCommand(PitchLibrary& library, std::string track_id)
        : library_(library), track_id_(std::move(track_id)) {}

    void execute() override {
        if (const PitchTrack* track = library_.find_track(track_id_)) {
            saved_track_ = *track;
        }
        library_.remove_track(track_id_);
    }

    void undo() override {
        library_.add_track(saved_track_);
    }

    [[nodiscard]] std::string description() const override {
        return "Remove pitch track";
    }

private:
    PitchLibrary& library_;
    std::string track_id_;
    PitchTrack saved_track_;
};

class ModifyPitchTrackCommand : public Command {
public:
    ModifyPitchTrackCommand(PitchLibrary& library, std::string track_id,
                            PitchTrack old_state, PitchTrack new_state,
                            std::string action_name = "Modify pitch track")
        : library_(library)
        , track_id_(std::move(track_id))
        , old_state_(std::move(old_state))
        , new_state_(std::move(new_state))
        , action_name_(std::move(action_name)) {}

    void execute() override {
        if (PitchTrack* track = library_.find_track(track_id_)) {
            *track = new_state_;
        }
    }

    void undo() override {
        if (PitchTrack* track = library_.find_track(track_id_)) {
            *track = old_state_;
        }
    }

    [[nodiscard]] std::string description() const override {
        return action_name_;
    }

private:
    PitchLibrary& library_;
    std::string track_id_;
    PitchTrack old_state_;
    PitchTrack new_state_;
    std::string action_name_;
};

} // namespace furious
