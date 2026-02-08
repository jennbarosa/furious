#include <gtest/gtest.h>
#include "furious/audio/audio_decoder.hpp"

namespace furious {
namespace {

class AudioDecoderTest : public ::testing::Test {
protected:
    AudioDecoder decoder;
};

TEST_F(AudioDecoderTest, DefaultStateNotOpen) {
    EXPECT_FALSE(decoder.is_open());
    EXPECT_FALSE(decoder.has_audio_stream());
    EXPECT_DOUBLE_EQ(decoder.duration_seconds(), 0.0);
}

TEST_F(AudioDecoderTest, OpenNonexistentFileFails) {
    auto result = decoder.open("/nonexistent/path/to/audio.wav");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(AudioDecoderTest, OpenEmptyPathFails) {
    auto result = decoder.open("");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(AudioDecoderTest, OpenInvalidFileFails) {
    auto result = decoder.open("/dev/null");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(AudioDecoderTest, CloseWhenNotOpenIsNoOp) {
    EXPECT_NO_THROW(decoder.close());
    EXPECT_FALSE(decoder.is_open());
}

TEST_F(AudioDecoderTest, ExtractAllWhenNotOpenFails) {
    auto result = decoder.extract_all();
    EXPECT_FALSE(result.has_value());
}


TEST_F(AudioDecoderTest, MultipleCloseCallsAreNoOp) {
    decoder.close();
    decoder.close();
    decoder.close();
    EXPECT_FALSE(decoder.is_open());
}

class AudioDecoderErrorTest : public ::testing::Test {
protected:
    AudioDecoder decoder;
};

TEST_F(AudioDecoderErrorTest, OpenReturnsErrorMessageForMissingFile) {
    auto result = decoder.open("/this/path/definitely/does/not/exist.mp3");
    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(AudioDecoderErrorTest, OpenDirectoryFails) {
    auto result = decoder.open("/tmp");
    EXPECT_FALSE(result.has_value());
}

} // namespace
} // namespace furious
