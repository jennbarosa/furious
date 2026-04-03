#include "furious/audio/pitch_estimator.hpp"
#include <cmath>
#include <algorithm>
#include <vector>

namespace furious {

namespace pitch_estimator_detail {

constexpr float A4_FREQUENCY = 440.0f;
constexpr int A4_MIDI = 69;

constexpr float MIN_FREQUENCY = 50.0f;
constexpr float MAX_FREQUENCY = 2000.0f;

} // namespace pitch_estimator_detail

PitchResult PitchEstimator::estimate(
    const float* samples,
    size_t num_samples,
    uint32_t sample_rate,
    PitchAlgorithm algorithm
) {
    if (!samples || num_samples < 128) {
        return {};
    }

    switch (algorithm) {
        case PitchAlgorithm::Autocorrelation:
            return autocorrelation(samples, num_samples, sample_rate);
        case PitchAlgorithm::YIN:
        default:
            return yin(samples, num_samples, sample_rate);
    }
}

int PitchEstimator::frequency_to_midi(float freq) {
    if (freq <= 0.0f) {
        return 0;
    }
    float midi = 69.0f + 12.0f * std::log2(freq / pitch_estimator_detail::A4_FREQUENCY);
    return static_cast<int>(std::round(midi));
}

PitchResult PitchEstimator::autocorrelation(
    const float* samples,
    size_t num_samples,
    uint32_t sample_rate
) {
    const size_t min_period = static_cast<size_t>(sample_rate / pitch_estimator_detail::MAX_FREQUENCY);
    const size_t max_period = static_cast<size_t>(sample_rate / pitch_estimator_detail::MIN_FREQUENCY);

    if (max_period >= num_samples / 2) {
        return {};
    }

    float energy = 0.0f;
    for (size_t i = 0; i < num_samples; ++i) {
        energy += samples[i] * samples[i];
    }
    if (energy < 0.0001f) {
        return {};
    }

    float best_correlation = 0.0f;
    size_t best_period = 0;

    for (size_t period = min_period; period < max_period && period < num_samples / 2; ++period) {
        float sum = 0.0f;
        for (size_t i = 0; i < num_samples - period; ++i) {
            sum += samples[i] * samples[i + period];
        }

        float normalized = sum / static_cast<float>(num_samples - period);

        if (normalized > best_correlation) {
            best_correlation = normalized;
            best_period = period;
        }
    }

    if (best_period == 0) {
        return {};
    }

    PitchResult result;
    result.frequency_hz = static_cast<float>(sample_rate) / static_cast<float>(best_period);
    result.confidence = best_correlation / (energy / static_cast<float>(num_samples));
    result.midi_note = frequency_to_midi(result.frequency_hz);

    return result;
}

PitchResult PitchEstimator::yin(
    const float* samples,
    size_t num_samples,
    uint32_t sample_rate
) {
    const size_t min_period = static_cast<size_t>(sample_rate / pitch_estimator_detail::MAX_FREQUENCY);
    const size_t max_period = std::min(
        static_cast<size_t>(sample_rate / pitch_estimator_detail::MIN_FREQUENCY),
        num_samples / 2
    );

    if (max_period <= min_period) {
        return {};
    }

    std::vector<float> difference(max_period);
    std::vector<float> cumulative_mean_normalized(max_period);

    for (size_t tau = 1; tau < max_period; ++tau) {
        float sum = 0.0f;
        for (size_t i = 0; i < num_samples - tau; ++i) {
            float delta = samples[i] - samples[i + tau];
            sum += delta * delta;
        }
        difference[tau] = sum;
    }

    cumulative_mean_normalized[0] = 1.0f;
    float running_sum = 0.0f;
    for (size_t tau = 1; tau < max_period; ++tau) {
        running_sum += difference[tau];
        if (running_sum > 0.0f) {
            cumulative_mean_normalized[tau] = difference[tau] * tau / running_sum;
        } else {
            cumulative_mean_normalized[tau] = 1.0f;
        }
    }

    constexpr float threshold = 0.1f;
    size_t best_period = 0;
    float best_value = 1.0f;

    for (size_t tau = min_period; tau < max_period - 1; ++tau) {
        if (cumulative_mean_normalized[tau] < threshold) {
            if (cumulative_mean_normalized[tau] < cumulative_mean_normalized[tau - 1] &&
                cumulative_mean_normalized[tau] <= cumulative_mean_normalized[tau + 1]) {
                best_period = tau;
                best_value = cumulative_mean_normalized[tau];
                break;
            }
        }
    }

    if (best_period == 0) {
        for (size_t tau = min_period; tau < max_period; ++tau) {
            if (cumulative_mean_normalized[tau] < best_value) {
                best_value = cumulative_mean_normalized[tau];
                best_period = tau;
            }
        }
    }

    if (best_period == 0 || best_value > 0.5f) {
        return {};
    }

    float refined_period = static_cast<float>(best_period);
    if (best_period > 0 && best_period < max_period - 1) {
        float y0 = cumulative_mean_normalized[best_period - 1];
        float y1 = cumulative_mean_normalized[best_period];
        float y2 = cumulative_mean_normalized[best_period + 1];
        float offset = (y0 - y2) / (2.0f * (y0 - 2.0f * y1 + y2));
        refined_period += offset;
    }

    PitchResult result;
    result.frequency_hz = static_cast<float>(sample_rate) / refined_period;
    result.confidence = 1.0f - best_value;
    result.midi_note = frequency_to_midi(result.frequency_hz);

    return result;
}

} // namespace furious
