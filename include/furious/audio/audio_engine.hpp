#pragma once

#include "furious/audio/audio_clip.hpp"
#include <memory>
#include <atomic>
#include <vector>

namespace furious {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool initialize();
    void shutdown();

    bool load_clip(const std::string& filepath);
    void unload_clip();

    void play();
    void pause();
    void stop();

    void set_playhead_seconds(double seconds);
    [[nodiscard]] double playhead_seconds() const;

    [[nodiscard]] bool is_playing() const { return is_playing_; }
    [[nodiscard]] bool has_clip() const { return clip_ && clip_->is_loaded(); }
    [[nodiscard]] const AudioClip* clip() const { return clip_.get(); }

    void set_metronome_enabled(bool enabled) { metronome_enabled_ = enabled; }
    [[nodiscard]] bool metronome_enabled() const { return metronome_enabled_; }
    void set_bpm(double bpm);
    [[nodiscard]] double bpm() const { return bpm_; }

    void set_beats_per_measure(int beats) { beats_per_measure_ = beats; }
    [[nodiscard]] int beats_per_measure() const { return beats_per_measure_; }

    void set_clip_start_seconds(double seconds);
    void set_clip_end_seconds(double seconds);
    [[nodiscard]] double clip_start_seconds() const { return clip_start_seconds_; }
    [[nodiscard]] double clip_end_seconds() const { return clip_end_seconds_; }
    [[nodiscard]] double trimmed_duration_seconds() const;
    void reset_clip_bounds();

    [[nodiscard]] uint64_t clip_start_frame() const;
    [[nodiscard]] uint64_t clip_end_frame() const;
    [[nodiscard]] uint64_t playhead_frame() const { return playhead_frame_.load(); }
    void advance_playhead(uint32_t frames);
    [[nodiscard]] const std::vector<float>& click_sound_high() const { return click_sound_high_; }
    [[nodiscard]] const std::vector<float>& click_sound_low() const { return click_sound_low_; }
    [[nodiscard]] uint32_t sample_rate() const { return sample_rate_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::unique_ptr<AudioClip> clip_;
    std::atomic<bool> is_playing_{false};
    std::atomic<uint64_t> playhead_frame_{0};
    std::atomic<bool> metronome_enabled_{false};
    std::atomic<double> bpm_{120.0};
    std::atomic<int> beats_per_measure_{4};
    std::vector<float> click_sound_high_;
    std::vector<float> click_sound_low_;
    uint32_t sample_rate_ = 44100;
    std::atomic<double> clip_start_seconds_{0.0};
    std::atomic<double> clip_end_seconds_{0.0};

    void generate_click_sounds();
};

} // namespace furious
