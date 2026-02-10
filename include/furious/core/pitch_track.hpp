#pragma once

#include <string>
#include <vector>

namespace furious {

struct PitchNote {
    int subdivision_index = 0;
    int midi_note = 60;
    float fine_tune_cents = 0.0f;

    [[nodiscard]] float frequency_hz() const;
    [[nodiscard]] std::string note_name() const;
};

struct PitchTrack {
    std::string id;
    std::string name;
    int length_subdivisions = 16;
    std::vector<PitchNote> notes;
    float smoothing = 0.15f;
    bool autotune_enabled = true;

    [[nodiscard]] const PitchNote* note_at(int subdivision) const;
    [[nodiscard]] float pitch_shift_cents_for(int subdivision, float source_freq_hz) const;
};

int note_name_to_midi(const std::string& name);
std::string midi_to_note_name(int midi);
float midi_to_frequency(int midi, float fine_cents = 0.0f);
float cents_between_frequencies(float freq1, float freq2);

} // namespace furious
