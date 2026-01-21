#include "furious/video/video_decoder.hpp"
#include "furious/video/video_engine.hpp"
#include "furious/core/media_source.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace furious;

class VideoDecoderTest : public ::testing::Test {
protected:
    VideoDecoder decoder;
};

TEST_F(VideoDecoderTest, NotOpenByDefault) {
    EXPECT_FALSE(decoder.is_open());
    EXPECT_EQ(decoder.width(), 0);
    EXPECT_EQ(decoder.height(), 0);
}

TEST_F(VideoDecoderTest, OpenNonexistentFileFails) {
    EXPECT_FALSE(decoder.open("/nonexistent/path/to/video.mp4"));
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(VideoDecoderTest, CloseDoesNotCrash) {
    decoder.close();
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(VideoDecoderTest, SeekAndDecodeWhenNotOpenFails) {
    std::vector<uint8_t> buffer;
    EXPECT_FALSE(decoder.seek_and_decode(0.0, buffer));
}

class VideoEngineTest : public ::testing::Test {
protected:
    VideoEngine engine;
};

TEST_F(VideoEngineTest, InitializeAndShutdown) {
    EXPECT_TRUE(engine.initialize());
    engine.shutdown();
}

TEST_F(VideoEngineTest, GetTextureForNonexistentClipReturnsZero) {
    engine.initialize();
    EXPECT_EQ(engine.get_texture("nonexistent_clip"), 0u);
}

TEST_F(VideoEngineTest, RequestFrameForNonexistentSourceDoesNotCrash) {
    engine.initialize();
    engine.request_frame("clip1", "nonexistent_source", 0.0);
}

TEST_F(VideoEngineTest, RegisterSourceWithNonexistentVideo) {
    engine.initialize();
    MediaSource source;
    source.id = "test-source";
    source.filepath = "/nonexistent/video.mp4";
    source.type = MediaType::Video;
    engine.register_source(source);
    EXPECT_EQ(engine.get_texture("clip1"), 0u);
}

TEST_F(VideoEngineTest, SetPlaying) {
    engine.initialize();
    engine.set_playing(true);
    engine.set_playing(false);
}

TEST(MediaSourceTest, DefaultValues) {
    MediaSource source;
    EXPECT_TRUE(source.id.empty());
    EXPECT_TRUE(source.filepath.empty());
    EXPECT_EQ(source.type, MediaType::Video);
    EXPECT_EQ(source.width, 0);
    EXPECT_EQ(source.height, 0);
}

TEST(MediaSourceTest, CanSetAllFields) {
    MediaSource source;
    source.id = "my-id";
    source.filepath = "/path/to/video.mp4";
    source.name = "My Video";
    source.type = MediaType::Image;
    source.duration_seconds = 120.5;
    source.width = 1920;
    source.height = 1080;
    source.fps = 60.0;

    EXPECT_EQ(source.id, "my-id");
    EXPECT_EQ(source.filepath, "/path/to/video.mp4");
    EXPECT_EQ(source.type, MediaType::Image);
    EXPECT_EQ(source.width, 1920);
    EXPECT_EQ(source.height, 1080);
}

TEST(MediaTypeTest, VideoAndImageAreDifferent) {
    EXPECT_NE(MediaType::Video, MediaType::Image);
}
