#include <gtest/gtest.h>
#include "furious/audio/audio_buffer.hpp"

namespace furious {
namespace {

class AudioBufferTest : public ::testing::Test {
protected:
    AudioBuffer create_stereo_buffer(size_t frame_count, uint32_t sample_rate = 44100) {
        std::vector<float> samples(frame_count * 2);
        for (size_t i = 0; i < samples.size(); ++i) {
            samples[i] = static_cast<float>(i) * 0.001f;
        }
        return AudioBuffer(std::move(samples), sample_rate, 2);
    }

    AudioBuffer create_mono_buffer(size_t frame_count, uint32_t sample_rate = 44100) {
        std::vector<float> samples(frame_count);
        for (size_t i = 0; i < samples.size(); ++i) {
            samples[i] = static_cast<float>(i) * 0.001f;
        }
        return AudioBuffer(std::move(samples), sample_rate, 1);
    }
};

TEST_F(AudioBufferTest, DefaultConstructorCreatesEmptyBuffer) {
    AudioBuffer buffer;
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.frame_count(), 0u);
    EXPECT_EQ(buffer.sample_rate(), 44100u);
    EXPECT_EQ(buffer.channels(), 2u);
}

TEST_F(AudioBufferTest, ConstructorWithSamples) {
    std::vector<float> samples = {0.1f, 0.2f, 0.3f, 0.4f};
    AudioBuffer buffer(std::move(samples), 48000, 2);

    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(buffer.sample_rate(), 48000u);
    EXPECT_EQ(buffer.channels(), 2u);
}

TEST_F(AudioBufferTest, FrameCountStereo) {
    auto buffer = create_stereo_buffer(100);
    EXPECT_EQ(buffer.frame_count(), 100u);
}

TEST_F(AudioBufferTest, FrameCountMono) {
    auto buffer = create_mono_buffer(100);
    EXPECT_EQ(buffer.frame_count(), 100u);
}

TEST_F(AudioBufferTest, FrameCountWithZeroChannels) {
    std::vector<float> samples = {0.1f, 0.2f};
    AudioBuffer buffer(std::move(samples), 44100, 0);
    EXPECT_EQ(buffer.frame_count(), 0u);
}

TEST_F(AudioBufferTest, DurationSecondsStereo) {
    auto buffer = create_stereo_buffer(44100);
    EXPECT_DOUBLE_EQ(buffer.duration_seconds(), 1.0);
}

TEST_F(AudioBufferTest, DurationSecondsMono) {
    auto buffer = create_mono_buffer(44100);
    EXPECT_DOUBLE_EQ(buffer.duration_seconds(), 1.0);
}

TEST_F(AudioBufferTest, DurationSecondsHalfSecond) {
    auto buffer = create_stereo_buffer(22050);
    EXPECT_DOUBLE_EQ(buffer.duration_seconds(), 0.5);
}

TEST_F(AudioBufferTest, DurationSecondsDifferentSampleRate) {
    std::vector<float> samples(48000 * 2);
    AudioBuffer buffer(std::move(samples), 48000, 2);
    EXPECT_DOUBLE_EQ(buffer.duration_seconds(), 1.0);
}

TEST_F(AudioBufferTest, DurationSecondsWithZeroSampleRate) {
    std::vector<float> samples = {0.1f, 0.2f};
    AudioBuffer buffer(std::move(samples), 0, 2);
    EXPECT_DOUBLE_EQ(buffer.duration_seconds(), 0.0);
}

TEST_F(AudioBufferTest, SamplesSpan) {
    std::vector<float> original = {0.1f, 0.2f, 0.3f, 0.4f};
    AudioBuffer buffer(std::vector<float>(original), 44100, 2);

    auto span = buffer.samples();
    EXPECT_EQ(span.size(), 4u);
    EXPECT_FLOAT_EQ(span[0], 0.1f);
    EXPECT_FLOAT_EQ(span[1], 0.2f);
    EXPECT_FLOAT_EQ(span[2], 0.3f);
    EXPECT_FLOAT_EQ(span[3], 0.4f);
}

TEST_F(AudioBufferTest, SampleAtValidIndex) {
    std::vector<float> samples = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
    AudioBuffer buffer(std::move(samples), 44100, 2);

    EXPECT_FLOAT_EQ(buffer.sample_at(0, 0), 0.1f);
    EXPECT_FLOAT_EQ(buffer.sample_at(0, 1), 0.2f);

    EXPECT_FLOAT_EQ(buffer.sample_at(1, 0), 0.3f);
    EXPECT_FLOAT_EQ(buffer.sample_at(1, 1), 0.4f);

    EXPECT_FLOAT_EQ(buffer.sample_at(2, 0), 0.5f);
    EXPECT_FLOAT_EQ(buffer.sample_at(2, 1), 0.6f);
}

TEST_F(AudioBufferTest, SampleAtInvalidChannel) {
    std::vector<float> samples = {0.1f, 0.2f};
    AudioBuffer buffer(std::move(samples), 44100, 2);

    EXPECT_FLOAT_EQ(buffer.sample_at(0, 2), 0.0f);
    EXPECT_FLOAT_EQ(buffer.sample_at(0, 100), 0.0f);
}

TEST_F(AudioBufferTest, SampleAtInvalidFrame) {
    std::vector<float> samples = {0.1f, 0.2f};
    AudioBuffer buffer(std::move(samples), 44100, 2);

    EXPECT_FLOAT_EQ(buffer.sample_at(1, 0), 0.0f);
    EXPECT_FLOAT_EQ(buffer.sample_at(100, 0), 0.0f);
}

TEST_F(AudioBufferTest, SampleAtMonoBuffer) {
    std::vector<float> samples = {0.1f, 0.2f, 0.3f};
    AudioBuffer buffer(std::move(samples), 44100, 1);

    EXPECT_FLOAT_EQ(buffer.sample_at(0, 0), 0.1f);
    EXPECT_FLOAT_EQ(buffer.sample_at(1, 0), 0.2f);
    EXPECT_FLOAT_EQ(buffer.sample_at(2, 0), 0.3f);
    EXPECT_FLOAT_EQ(buffer.sample_at(0, 1), 0.0f);
}

TEST_F(AudioBufferTest, EmptyReturnsTrue) {
    AudioBuffer buffer;
    EXPECT_TRUE(buffer.empty());
}

TEST_F(AudioBufferTest, EmptyReturnsFalseWithSamples) {
    std::vector<float> samples = {0.1f};
    AudioBuffer buffer(std::move(samples), 44100, 1);
    EXPECT_FALSE(buffer.empty());
}

TEST_F(AudioBufferTest, MoveConstruction) {
    std::vector<float> samples = {0.1f, 0.2f, 0.3f, 0.4f};
    AudioBuffer original(std::move(samples), 48000, 2);

    AudioBuffer moved(std::move(original));

    EXPECT_EQ(moved.frame_count(), 2u);
    EXPECT_EQ(moved.sample_rate(), 48000u);
    EXPECT_EQ(moved.channels(), 2u);
    EXPECT_FLOAT_EQ(moved.sample_at(0, 0), 0.1f);
}

TEST_F(AudioBufferTest, MoveAssignment) {
    std::vector<float> samples = {0.1f, 0.2f, 0.3f, 0.4f};
    AudioBuffer original(std::move(samples), 48000, 2);

    AudioBuffer moved;
    moved = std::move(original);

    EXPECT_EQ(moved.frame_count(), 2u);
    EXPECT_EQ(moved.sample_rate(), 48000u);
    EXPECT_EQ(moved.channels(), 2u);
}

TEST_F(AudioBufferTest, LargeBuffer) {
    const size_t frame_count = 44100 * 60;
    auto buffer = create_stereo_buffer(frame_count);

    EXPECT_EQ(buffer.frame_count(), frame_count);
    EXPECT_DOUBLE_EQ(buffer.duration_seconds(), 60.0);
}

TEST_F(AudioBufferTest, MultichannelBuffer) {
    std::vector<float> samples(480);
    for (size_t i = 0; i < samples.size(); ++i) {
        samples[i] = static_cast<float>(i % 6);
    }
    AudioBuffer buffer(std::move(samples), 44100, 6);

    EXPECT_EQ(buffer.channels(), 6u);
    EXPECT_EQ(buffer.frame_count(), 80u);

    EXPECT_FLOAT_EQ(buffer.sample_at(0, 0), 0.0f);
    EXPECT_FLOAT_EQ(buffer.sample_at(0, 5), 5.0f);
    EXPECT_FLOAT_EQ(buffer.sample_at(1, 0), 0.0f);
}

} // namespace
} // namespace furious
