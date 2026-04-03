#include "furious/audio/pitch_shifter.hpp"
#include <rubberband/RubberBandStretcher.h>
#include <cmath>
#include <algorithm>

namespace furious {

struct PitchShifter::Impl {
    std::unique_ptr<RubberBand::RubberBandStretcher> stretcher;
    uint32_t sample_rate = 44100;
    uint32_t channels = 2;
    float current_cents = 0.0f;

    std::vector<std::vector<float>> input_channels;
    std::vector<std::vector<float>> output_channels;
    std::vector<const float*> input_ptrs;
    std::vector<float*> output_ptrs;

    void ensure_buffers(size_t num_frames) {
        for (uint32_t c = 0; c < channels; ++c) {
            if (input_channels[c].size() < num_frames) {
                input_channels[c].resize(num_frames);
                output_channels[c].resize(num_frames);
            }
        }
    }

    void deinterleave(const float* interleaved, size_t num_frames) {
        for (size_t i = 0; i < num_frames; ++i) {
            for (uint32_t c = 0; c < channels; ++c) {
                input_channels[c][i] = interleaved[i * channels + c];
            }
        }
    }

    void interleave(float* interleaved, size_t num_frames) {
        for (size_t i = 0; i < num_frames; ++i) {
            for (uint32_t c = 0; c < channels; ++c) {
                interleaved[i * channels + c] = output_channels[c][i];
            }
        }
    }
};

PitchShifter::PitchShifter(uint32_t sample_rate, uint32_t channels)
    : impl_(std::make_unique<Impl>()) {
    impl_->sample_rate = sample_rate;
    impl_->channels = channels;

    RubberBand::RubberBandStretcher::Options options =
        RubberBand::RubberBandStretcher::OptionProcessRealTime |
        RubberBand::RubberBandStretcher::OptionPitchHighConsistency |
        RubberBand::RubberBandStretcher::OptionChannelsTogether;

    impl_->stretcher = std::make_unique<RubberBand::RubberBandStretcher>(
        sample_rate, channels, options
    );

    impl_->input_channels.resize(channels);
    impl_->output_channels.resize(channels);
    impl_->input_ptrs.resize(channels);
    impl_->output_ptrs.resize(channels);

    for (uint32_t c = 0; c < channels; ++c) {
        impl_->input_ptrs[c] = impl_->input_channels[c].data();
        impl_->output_ptrs[c] = impl_->output_channels[c].data();
    }
}

PitchShifter::~PitchShifter() = default;

PitchShifter::PitchShifter(PitchShifter&&) noexcept = default;
PitchShifter& PitchShifter::operator=(PitchShifter&&) noexcept = default;

void PitchShifter::set_pitch_shift_cents(float cents) {
    if (std::abs(cents - impl_->current_cents) < 0.01f) {
        return;
    }

    double ratio = std::pow(2.0, static_cast<double>(cents) / 1200.0);
    impl_->stretcher->setPitchScale(ratio);
    impl_->current_cents = cents;
}

float PitchShifter::pitch_shift_cents() const {
    return impl_->current_cents;
}

void PitchShifter::process(float* samples, size_t num_frames) {
    process(samples, samples, num_frames);
}

void PitchShifter::process(const float* input, float* output, size_t num_frames) {
    if (num_frames == 0) {
        return;
    }

    if (std::abs(impl_->current_cents) < 1.0f) {
        if (input != output) {
            std::copy(input, input + num_frames * impl_->channels, output);
        }
        return;
    }

    impl_->ensure_buffers(num_frames);
    impl_->deinterleave(input, num_frames);

    for (uint32_t c = 0; c < impl_->channels; ++c) {
        impl_->input_ptrs[c] = impl_->input_channels[c].data();
        impl_->output_ptrs[c] = impl_->output_channels[c].data();
    }

    impl_->stretcher->process(impl_->input_ptrs.data(), num_frames, false);

    size_t available = static_cast<size_t>(impl_->stretcher->available());
    if (available > 0) {
        size_t to_retrieve = std::min(available, num_frames);
        impl_->stretcher->retrieve(impl_->output_ptrs.data(), to_retrieve);

        impl_->interleave(output, to_retrieve);

        if (to_retrieve < num_frames) {
            std::fill(output + to_retrieve * impl_->channels,
                     output + num_frames * impl_->channels, 0.0f);
        }
    } else {
        std::fill(output, output + num_frames * impl_->channels, 0.0f);
    }
}

std::vector<float> PitchShifter::process_offline(const float* input, size_t num_frames) {
    RubberBand::RubberBandStretcher::Options options =
        RubberBand::RubberBandStretcher::OptionProcessOffline |
        RubberBand::RubberBandStretcher::OptionPitchHighQuality |
        RubberBand::RubberBandStretcher::OptionChannelsTogether;

    RubberBand::RubberBandStretcher offline_stretcher(
        impl_->sample_rate, impl_->channels, options
    );

    double ratio = std::pow(2.0, static_cast<double>(impl_->current_cents) / 1200.0);
    offline_stretcher.setPitchScale(ratio);

    std::vector<std::vector<float>> channels(impl_->channels);
    std::vector<const float*> in_ptrs(impl_->channels);

    for (uint32_t c = 0; c < impl_->channels; ++c) {
        channels[c].resize(num_frames);
        for (size_t i = 0; i < num_frames; ++i) {
            channels[c][i] = input[i * impl_->channels + c];
        }
        in_ptrs[c] = channels[c].data();
    }

    offline_stretcher.study(in_ptrs.data(), num_frames, true);

    offline_stretcher.process(in_ptrs.data(), num_frames, true);

    size_t available = static_cast<size_t>(offline_stretcher.available());

    std::vector<std::vector<float>> out_channels(impl_->channels);
    std::vector<float*> out_ptrs(impl_->channels);

    for (uint32_t c = 0; c < impl_->channels; ++c) {
        out_channels[c].resize(available);
        out_ptrs[c] = out_channels[c].data();
    }

    offline_stretcher.retrieve(out_ptrs.data(), available);

    std::vector<float> result(available * impl_->channels);
    for (size_t i = 0; i < available; ++i) {
        for (uint32_t c = 0; c < impl_->channels; ++c) {
            result[i * impl_->channels + c] = out_channels[c][i];
        }
    }

    return result;
}

void PitchShifter::prepare(size_t max_frames) {
    impl_->ensure_buffers(max_frames);
}

void PitchShifter::reset() {
    impl_->stretcher->reset();
}

uint32_t PitchShifter::sample_rate() const {
    return impl_->sample_rate;
}

uint32_t PitchShifter::channels() const {
    return impl_->channels;
}

} // namespace furious
