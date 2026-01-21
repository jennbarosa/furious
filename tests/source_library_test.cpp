#include "furious/video/source_library.hpp"
#include "furious/core/media_source.hpp"
#include <gtest/gtest.h>

using namespace furious;

class SourceLibraryTest : public ::testing::Test {
protected:
    SourceLibrary library;
};

TEST_F(SourceLibraryTest, StartsEmpty) {
    EXPECT_EQ(library.source_count(), 0u);
    EXPECT_TRUE(library.sources().empty());
}

TEST_F(SourceLibraryTest, AddSource) {
    std::string id = library.add_source("/path/to/video.mp4");
    EXPECT_EQ(library.source_count(), 1u);
    EXPECT_FALSE(id.empty());
}

TEST_F(SourceLibraryTest, AddSourceExtractsFilename) {
    std::string id = library.add_source("/home/user/videos/my_video.mp4");
    const MediaSource* src = library.find_source(id);
    ASSERT_NE(src, nullptr);
    EXPECT_EQ(src->name, "my_video.mp4");
}

TEST_F(SourceLibraryTest, DetectsVideoType) {
    std::string id = library.add_source("/path/video.mp4");
    EXPECT_EQ(library.find_source(id)->type, MediaType::Video);
}

TEST_F(SourceLibraryTest, DetectsImageType) {
    std::string id = library.add_source("/path/image.png");
    EXPECT_EQ(library.find_source(id)->type, MediaType::Image);
}

TEST_F(SourceLibraryTest, DetectsTypeCaseInsensitive) {
    std::string id1 = library.add_source("/path/image.PNG");
    std::string id2 = library.add_source("/path/video.MP4");
    EXPECT_EQ(library.find_source(id1)->type, MediaType::Image);
    EXPECT_EQ(library.find_source(id2)->type, MediaType::Video);
}

TEST_F(SourceLibraryTest, FindSource) {
    std::string id = library.add_source("/path/video.mp4");
    EXPECT_NE(library.find_source(id), nullptr);
    EXPECT_EQ(library.find_source("nonexistent"), nullptr);
}

TEST_F(SourceLibraryTest, RemoveSource) {
    std::string id = library.add_source("/path/video.mp4");
    library.remove_source(id);
    EXPECT_EQ(library.source_count(), 0u);
    EXPECT_EQ(library.find_source(id), nullptr);
}

TEST_F(SourceLibraryTest, Clear) {
    library.add_source("/video1.mp4");
    library.add_source("/video2.mp4");
    library.clear();
    EXPECT_EQ(library.source_count(), 0u);
}

TEST_F(SourceLibraryTest, AddSourceDirect) {
    MediaSource source;
    source.id = "preserved-id";
    source.filepath = "/path/video.mp4";
    source.name = "My Video";
    source.type = MediaType::Video;
    source.width = 1920;
    source.height = 1080;

    library.add_source_direct(source);

    const MediaSource* found = library.find_source("preserved-id");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "My Video");
    EXPECT_EQ(found->width, 1920);
}
