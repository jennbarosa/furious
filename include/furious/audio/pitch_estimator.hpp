#pragma once

#include <cstddef>
#include <cstdint>

namespace furious {

enum class PitchAlgorithm {
    Autocorrelation,
    YIN
};

struct PitchResult {
    float frequency_hz = 0.0f;
    float confidence = 0.0f;
    int midi_note = 0;
};

class PitchEstimator {
public:
    static PitchResult estimate(
        const float* samples,
        size_t num_samples,
        uint32_t sample_rate,
        PitchAlgorithm algorithm = PitchAlgorithm::YIN
    );

    static int frequency_to_midi(float freq);

private:
    static PitchResult autocorrelation(const float* samples, size_t num_samples, uint32_t sample_rate);
    static PitchResult yin(const float* samples, size_t num_samples, uint32_t sample_rate);
};

} // namespace furious
