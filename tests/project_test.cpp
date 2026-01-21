#include "furious/core/project.hpp"
#include <gtest/gtest.h>

using namespace furious;

class ProjectTest : public ::testing::Test {
protected:
    Project project;
};

TEST_F(ProjectTest, DefaultName) {
    EXPECT_EQ(project.name(), "Untitled Project");
}

TEST_F(ProjectTest, NameCanBeChanged) {
    project.set_name("My Project");
    EXPECT_EQ(project.name(), "My Project");
}

TEST_F(ProjectTest, DefaultTempoIs120) {
    EXPECT_DOUBLE_EQ(project.tempo().bpm(), 120.0);
}

TEST_F(ProjectTest, TempoCanBeModified) {
    project.tempo().set_bpm(140.0);
    EXPECT_DOUBLE_EQ(project.tempo().bpm(), 140.0);
}

TEST_F(ProjectTest, DefaultGridSubdivisionIsQuarter) {
    EXPECT_EQ(project.grid_subdivision(), NoteSubdivision::Quarter);
}

TEST_F(ProjectTest, GridSubdivisionCanBeChanged) {
    project.set_grid_subdivision(NoteSubdivision::Eighth);
    EXPECT_EQ(project.grid_subdivision(), NoteSubdivision::Eighth);
    project.set_grid_subdivision(NoteSubdivision::Sixteenth);
    EXPECT_EQ(project.grid_subdivision(), NoteSubdivision::Sixteenth);
}

TEST_F(ProjectTest, DefaultFpsIs30) {
    EXPECT_DOUBLE_EQ(project.fps(), 30.0);
}

TEST_F(ProjectTest, FpsCanBeSet) {
    project.set_fps(60.0);
    EXPECT_DOUBLE_EQ(project.fps(), 60.0);
}
