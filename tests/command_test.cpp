#include "furious/core/command.hpp"
#include "furious/core/clip_commands.hpp"
#include "furious/core/timeline_data.hpp"
#include <gtest/gtest.h>

using namespace furious;

class CommandHistoryTest : public ::testing::Test {
protected:
    CommandHistory history;
    TimelineData data;
};

TEST_F(CommandHistoryTest, StartsEmpty) {
    EXPECT_FALSE(history.can_undo());
    EXPECT_FALSE(history.can_redo());
}

TEST_F(CommandHistoryTest, CanUndoAfterExecute) {
    TimelineClip clip;
    clip.id = "test-clip";
    auto cmd = std::make_unique<AddClipCommand>(data, clip);
    history.execute(std::move(cmd));

    EXPECT_TRUE(history.can_undo());
    EXPECT_FALSE(history.can_redo());
    EXPECT_EQ(data.clips().size(), 1u);
}

TEST_F(CommandHistoryTest, UndoRevertsAddClip) {
    TimelineClip clip;
    clip.id = "test-clip";
    auto cmd = std::make_unique<AddClipCommand>(data, clip);
    history.execute(std::move(cmd));

    history.undo();

    EXPECT_EQ(data.clips().size(), 0u);
    EXPECT_FALSE(history.can_undo());
    EXPECT_TRUE(history.can_redo());
}

TEST_F(CommandHistoryTest, RedoReappliesAddClip) {
    TimelineClip clip;
    clip.id = "test-clip";
    auto cmd = std::make_unique<AddClipCommand>(data, clip);
    history.execute(std::move(cmd));
    history.undo();

    history.redo();

    EXPECT_EQ(data.clips().size(), 1u);
    EXPECT_TRUE(history.can_undo());
    EXPECT_FALSE(history.can_redo());
}

TEST_F(CommandHistoryTest, NewCommandClearsRedoStack) {
    TimelineClip clip1;
    clip1.id = "clip-1";
    auto cmd1 = std::make_unique<AddClipCommand>(data, clip1);
    history.execute(std::move(cmd1));
    history.undo();

    EXPECT_TRUE(history.can_redo());

    TimelineClip clip2;
    clip2.id = "clip-2";
    auto cmd2 = std::make_unique<AddClipCommand>(data, clip2);
    history.execute(std::move(cmd2));

    EXPECT_FALSE(history.can_redo());
}

TEST_F(CommandHistoryTest, RemoveClipCanBeUndone) {
    TimelineClip clip;
    clip.id = "test-clip";
    clip.start_beat = 4.0;
    data.add_clip(clip);

    auto cmd = std::make_unique<RemoveClipCommand>(data, "test-clip");
    history.execute(std::move(cmd));

    EXPECT_EQ(data.clips().size(), 0u);

    history.undo();

    EXPECT_EQ(data.clips().size(), 1u);
    EXPECT_DOUBLE_EQ(data.clips()[0].start_beat, 4.0);
}

TEST_F(CommandHistoryTest, ModifyClipCanBeUndone) {
    TimelineClip clip;
    clip.id = "test-clip";
    clip.start_beat = 0.0;
    data.add_clip(clip);

    TimelineClip old_state = clip;
    TimelineClip new_state = clip;
    new_state.start_beat = 8.0;

    auto cmd = std::make_unique<ModifyClipCommand>(data, "test-clip", old_state, new_state);
    history.execute(std::move(cmd));

    EXPECT_DOUBLE_EQ(data.find_clip("test-clip")->start_beat, 8.0);

    history.undo();

    EXPECT_DOUBLE_EQ(data.find_clip("test-clip")->start_beat, 0.0);
}

TEST_F(CommandHistoryTest, ModifyClipCanBeRedone) {
    TimelineClip clip;
    clip.id = "test-clip";
    clip.start_beat = 0.0;
    data.add_clip(clip);

    TimelineClip old_state = clip;
    TimelineClip new_state = clip;
    new_state.start_beat = 8.0;

    auto cmd = std::make_unique<ModifyClipCommand>(data, "test-clip", old_state, new_state);
    history.execute(std::move(cmd));
    history.undo();
    history.redo();

    EXPECT_DOUBLE_EQ(data.find_clip("test-clip")->start_beat, 8.0);
}

TEST_F(CommandHistoryTest, ClearRemovesAllHistory) {
    TimelineClip clip;
    clip.id = "test-clip";
    auto cmd = std::make_unique<AddClipCommand>(data, clip);
    history.execute(std::move(cmd));
    history.undo();

    EXPECT_TRUE(history.can_redo());

    history.clear();

    EXPECT_FALSE(history.can_undo());
    EXPECT_FALSE(history.can_redo());
}

TEST_F(CommandHistoryTest, MultipleUndoRedo) {
    TimelineClip clip1;
    clip1.id = "clip-1";
    clip1.start_beat = 0.0;

    TimelineClip clip2;
    clip2.id = "clip-2";
    clip2.start_beat = 4.0;

    history.execute(std::make_unique<AddClipCommand>(data, clip1));
    history.execute(std::make_unique<AddClipCommand>(data, clip2));

    EXPECT_EQ(data.clips().size(), 2u);

    history.undo();
    EXPECT_EQ(data.clips().size(), 1u);

    history.undo();
    EXPECT_EQ(data.clips().size(), 0u);

    history.redo();
    EXPECT_EQ(data.clips().size(), 1u);

    history.redo();
    EXPECT_EQ(data.clips().size(), 2u);
}

TEST_F(CommandHistoryTest, UndoDescriptionReturnsCorrectText) {
    TimelineClip clip;
    clip.id = "test-clip";
    auto cmd = std::make_unique<AddClipCommand>(data, clip);
    history.execute(std::move(cmd));

    EXPECT_EQ(history.undo_description(), "Add clip");
}

TEST_F(CommandHistoryTest, RedoDescriptionReturnsCorrectText) {
    TimelineClip clip;
    clip.id = "test-clip";
    auto cmd = std::make_unique<AddClipCommand>(data, clip);
    history.execute(std::move(cmd));
    history.undo();

    EXPECT_EQ(history.redo_description(), "Add clip");
}

TEST_F(CommandHistoryTest, EmptyHistoryDescriptionsReturnEmpty) {
    EXPECT_EQ(history.undo_description(), "");
    EXPECT_EQ(history.redo_description(), "");
}
