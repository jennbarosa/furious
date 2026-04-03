#pragma once

#include <memory>
#include <vector>
#include <cstdint>

namespace furious {

class PitchShifter {
public:
    PitchShifter(uint32_t sample_rate, uint32_t channels);
    ~PitchShifter();

    PitchShifter(const PitchShifter&) = delete;
    PitchShifter& operator=(const PitchShifter&) = delete;
    PitchShifter(PitchShifter&&) noexcept;
    PitchShifter& operator=(PitchShifter&&) noexcept;

    void set_pitch_shift_cents(float cents);
    [[nodiscard]] float pitch_shift_cents() const;

    void process(float* samples, size_t num_frames);

    void process(const float* input, float* output, size_t num_frames);

    std::vector<float> process_offline(const float* input, size_t num_frames);

    void prepare(size_t max_frames);

    void reset();

    [[nodiscard]] uint32_t sample_rate() const;
    [[nodiscard]] uint32_t channels() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace furious
