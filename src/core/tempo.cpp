#include "furious/core/tempo.hpp"
#include <algorithm>
#include <cmath>

namespace furious {

Tempo::Tempo(double bpm) : bpm_(std::clamp(bpm, 1.0, 999.0)) {}

void Tempo::set_bpm(double bpm) {
    bpm_ = std::clamp(bpm, 1.0, 999.0);
}

double Tempo::bpm() const {
    return bpm_;
}

double Tempo::beat_duration_seconds() const {
    return 60.0 / bpm_;
}

double Tempo::subdivision_duration_seconds(NoteSubdivision subdivision) const {
    return beat_duration_seconds() / static_cast<double>(subdivision);
}

double Tempo::time_to_beats(double seconds) const {
    return seconds / beat_duration_seconds();
}

double Tempo::beats_to_time(double beats) const {
    return beats * beat_duration_seconds();
}

int64_t Tempo::time_to_subdivision(double seconds, NoteSubdivision subdivision) const {
    double subdivision_duration = subdivision_duration_seconds(subdivision);
    return static_cast<int64_t>(std::floor(seconds / subdivision_duration));
}

} // namespace furious
