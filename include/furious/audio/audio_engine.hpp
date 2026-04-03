#pragma once

#include "furious/audio/audio_clip.hpp"
#include "furious/audio/audio_buffer.hpp"
#include <memory>
#include <atomic>
#include <vector>
#include <string>
#include <cassert>
#include <cstdint>

namespace furious {

class PitchShifter;

struct ClipAudioState {
    std::shared_ptr<const AudioBuffer> buffer;
    int64_t timeline_start_frame = 0;
    int64_t source_offset_frames = 0;
    int64_t duration_frames = 0;
    float volume = 1.0f;

    bool use_looped_audio = false;
    int64_t loop_start_frames = 0;
    int64_t loop_duration_frames = 0;
    int64_t loop_phase_offset_frames = 0;

    float pitch_shift_cents = 0.0f;

    std::string clip_id;
    bool autotune_enabled = false;
    int autotune_target_midi = 60;
    float autotune_amount = 1.0f;

    PitchShifter* pitch_shifter = nullptr;
};

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
    [[nodiscard]] bool has_clip() const {
        auto c = clip_.load();
        return c && c->is_loaded();
    }
    [[nodiscard]] const AudioClip* clip() const { return clip_.load().get(); }

    void set_metronome_enabled(bool enabled) { metronome_enabled_ = enabled; }
    [[nodiscard]] bool metronome_enabled() const { return metronome_enabled_; }

    void set_bgm_volume(float vol) { bgm_volume_ = vol; }
    [[nodiscard]] float bgm_volume() const { return bgm_volume_; }
    void set_clip_volume(float vol) { clip_volume_ = vol; }
    [[nodiscard]] float clip_volume() const { return clip_volume_; }
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

    void set_active_clips(std::vector<ClipAudioState> clips);
    void swap_active_clips_if_pending();
    [[nodiscard]] const std::vector<ClipAudioState>& active_clips() const { return clip_buffers_[read_index_]; }

    void* get_pitch_shifter_for_clip(const std::string& clip_id);
    void reset_pitch_shifter(const std::string& clip_id);
    void clear_pitch_shifters();

    void mix_bgm(float* output, uint32_t frame_count, uint64_t current_frame);

    static constexpr size_t kMaxExpectedFrames = 2048;
    [[nodiscard]] float* scratch_mono() { return scratch_mono_.data(); }
    [[nodiscard]] uint32_t* scratch_indices() { return scratch_indices_.data(); }
    [[nodiscard]] float* scratch_processed() { return scratch_processed_.data(); }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::atomic<std::shared_ptr<AudioClip>> clip_;
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
    std::atomic<float> bgm_volume_{1.0f};
    std::atomic<float> clip_volume_{1.0f};

    // Lock-free triple-buffer for active clips.
    // UI thread writes to clip_buffers_[write_index_], then publishes via
    // latest_index_.exchange(write_index_). Audio thread picks up the latest
    // buffer via read_index_ = latest_index_.exchange(read_index_).
    static constexpr int kNumClipBuffers = 3;
    std::vector<ClipAudioState> clip_buffers_[kNumClipBuffers];
    int read_index_ = 0;
    int write_index_ = 1;
    std::atomic<int> latest_index_{2};

    // Pre-allocated scratch buffers for the audio callback.
    std::vector<float> scratch_mono_;
    std::vector<uint32_t> scratch_indices_;
    std::vector<float> scratch_processed_;

    void generate_click_sounds();
};

} // namespace furious
