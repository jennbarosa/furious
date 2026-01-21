#pragma once

#include <cstdint>

namespace furious {

enum class NoteSubdivision {
    Quarter = 1,
    Eighth = 2,
    Sixteenth = 4
};

class Tempo {
public:
    explicit Tempo(double bpm = 120.0);

    void set_bpm(double bpm);
    [[nodiscard]] double bpm() const;

    [[nodiscard]] double beat_duration_seconds() const;
    [[nodiscard]] double subdivision_duration_seconds(NoteSubdivision subdivision) const;

    [[nodiscard]] double time_to_beats(double seconds) const;
    [[nodiscard]] double beats_to_time(double beats) const;

    [[nodiscard]] int64_t time_to_subdivision(double seconds, NoteSubdivision subdivision) const;

private:
    double bpm_;
};

} // namespace furious
