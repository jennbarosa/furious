#include "furious/core/pitch_track.hpp"
#include <cmath>
#include <array>
#include <algorithm>

namespace furious {

namespace {

constexpr std::array<const char*, 12> NOTE_NAMES = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

constexpr float A4_FREQUENCY = 440.0f;
constexpr int A4_MIDI = 69;

}

float PitchNote::frequency_hz() const {
    return midi_to_frequency(midi_note, fine_tune_cents);
}

std::string PitchNote::note_name() const {
    return midi_to_note_name(midi_note);
}

const PitchNote* PitchTrack::note_at(int subdivision) const {
    const PitchNote* result = nullptr;
    for (const auto& note : notes) {
        if (note.subdivision_index <= subdivision) {
            if (!result || note.subdivision_index > result->subdivision_index) {
                result = &note;
            }
        }
    }
    return result;
}

float PitchTrack::pitch_shift_cents_for(int subdivision, float source_freq_hz) const {
    const PitchNote* note = note_at(subdivision);
    if (!note || source_freq_hz <= 0.0f) {
        return 0.0f;
    }
    return cents_between_frequencies(source_freq_hz, note->frequency_hz());
}

int note_name_to_midi(const std::string& name) {
    if (name.empty()) {
        return 60;
    }

    size_t pos = 0;
    int note_index = 0;

    char base = std::toupper(name[0]);
    switch (base) {
        case 'C': note_index = 0; break;
        case 'D': note_index = 2; break;
        case 'E': note_index = 4; break;
        case 'F': note_index = 5; break;
        case 'G': note_index = 7; break;
        case 'A': note_index = 9; break;
        case 'B': note_index = 11; break;
        default: return 60;
    }
    pos = 1;

    if (pos < name.size()) {
        if (name[pos] == '#') {
            note_index++;
            pos++;
        } else if (name[pos] == 'b') {
            note_index--;
            pos++;
        }
    }

    int octave = 4;
    if (pos < name.size()) {
        try {
            octave = std::stoi(name.substr(pos));
        } catch (...) {
        }
    }

    return (octave + 1) * 12 + note_index;
}

std::string midi_to_note_name(int midi) {
    if (midi < 0 || midi > 127) {
        return "C4";
    }

    int note_index = midi % 12;
    int octave = (midi / 12) - 1;

    return std::string(NOTE_NAMES[note_index]) + std::to_string(octave);
}

float midi_to_frequency(int midi, float fine_cents) {
    float semitones = static_cast<float>(midi - A4_MIDI) + fine_cents / 100.0f;
    return A4_FREQUENCY * std::pow(2.0f, semitones / 12.0f);
}

float cents_between_frequencies(float freq1, float freq2) {
    if (freq1 <= 0.0f || freq2 <= 0.0f) {
        return 0.0f;
    }
    return 1200.0f * std::log2(freq2 / freq1);
}

} // namespace furious
