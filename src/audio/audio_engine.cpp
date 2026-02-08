#include "furious/audio/audio_engine.hpp"
#include "miniaudio.h"
#include <cmath>

namespace furious {

struct AudioEngine::Impl {
    ma_device device;
    bool device_initialized = false;
    uint64_t metronome_click_position = 0;  // (0 = not playing)
};

static void audio_callback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frame_count) {
    auto* engine = static_cast<AudioEngine*>(device->pUserData);
    auto* out = static_cast<float*>(output);

    bool has_clip = engine->has_clip();
    bool is_playing = engine->is_playing();

    std::fill(out, out + frame_count * 2, 0.0f);

    if (!is_playing) {
        return;
    }

    uint64_t current_frame = engine->playhead_frame();
    uint32_t sample_rate = engine->sample_rate();

    engine->swap_active_clips_if_pending();

    const auto& active_clips = engine->active_clips();
    for (const auto& clip_state : active_clips) {
        if (!clip_state.buffer || clip_state.buffer->empty()) continue;

        uint32_t channels = clip_state.buffer->channels();

        for (ma_uint32 i = 0; i < frame_count; ++i) {
            int64_t global_frame = static_cast<int64_t>(current_frame + i);
            int64_t frame_in_clip = global_frame - clip_state.timeline_start_frame;

            if (frame_in_clip < 0 || frame_in_clip >= clip_state.duration_frames) {
                continue;
            }

            int64_t source_frame;
            if (clip_state.use_looped_audio && clip_state.loop_duration_frames > 0) {
                int64_t adjusted_frame = frame_in_clip + clip_state.loop_phase_offset_frames;
                int64_t position_in_loop = adjusted_frame % clip_state.loop_duration_frames;
                if (position_in_loop < 0) position_in_loop += clip_state.loop_duration_frames;
                source_frame = clip_state.loop_start_frames + position_in_loop;
            } else {
                source_frame = clip_state.source_offset_frames + frame_in_clip;
            }

            if (source_frame < 0 || source_frame >= static_cast<int64_t>(clip_state.buffer->frame_count())) {
                continue;
            }

            float left = clip_state.buffer->sample_at(static_cast<uint64_t>(source_frame), 0);
            float right = channels >= 2
                ? clip_state.buffer->sample_at(static_cast<uint64_t>(source_frame), 1)
                : left;

            out[i * 2] += left * clip_state.volume;
            out[i * 2 + 1] += right * clip_state.volume;
        }
    }

    if (has_clip) {
        const AudioClip* clip = engine->clip();
        uint32_t channels = clip->channels();
        uint64_t start_frame = engine->clip_start_frame();
        uint64_t end_frame = engine->clip_end_frame();
        uint64_t trimmed_duration = end_frame - start_frame;

        for (ma_uint32 i = 0; i < frame_count; ++i) {
            uint64_t playhead_pos = current_frame + i;
            if (playhead_pos < trimmed_duration) {
                uint64_t clip_frame = start_frame + playhead_pos;
                if (clip_frame < clip->total_frames()) {
                    size_t sample_index = clip_frame * channels;
                    if (channels >= 2) {
                        out[i * 2] += clip->data()[sample_index];
                        out[i * 2 + 1] += clip->data()[sample_index + 1];
                    } else {
                        out[i * 2] += clip->data()[sample_index];
                        out[i * 2 + 1] += clip->data()[sample_index];
                    }
                }
            }
        }
    }

    if (engine->metronome_enabled()) {
        double bpm = engine->bpm();
        double samples_per_beat = (60.0 / bpm) * sample_rate;
        int beats_per_measure = engine->beats_per_measure();
        const auto& click_high = engine->click_sound_high();
        const auto& click_low = engine->click_sound_low();

        if (!click_high.empty() && !click_low.empty()) {
            for (ma_uint32 i = 0; i < frame_count; ++i) {
                uint64_t absolute_frame = current_frame + i;
                int beat_number = static_cast<int>(
                    std::floor(static_cast<double>(absolute_frame) / samples_per_beat)
                );
                uint64_t beat_frame = static_cast<uint64_t>(beat_number * samples_per_beat);
                uint64_t click_offset = absolute_frame - beat_frame;
                bool is_downbeat = (beat_number % beats_per_measure) == 0;
                const auto& click = is_downbeat ? click_high : click_low;

                if (click_offset < click.size()) {
                    float click_sample = click[click_offset];
                    out[i * 2] += click_sample;
                    out[i * 2 + 1] += click_sample;
                }
            }
        }
    }

    engine->advance_playhead(frame_count);
}

AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {
    generate_click_sounds();
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = sample_rate_;
    config.dataCallback = audio_callback;
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, &impl_->device) != MA_SUCCESS) {
        return false;
    }

    impl_->device_initialized = true;

    if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        ma_device_uninit(&impl_->device);
        impl_->device_initialized = false;
        return false;
    }

    return true;
}

void AudioEngine::generate_click_sounds() {
    const uint32_t click_samples = sample_rate_ / 50;
    constexpr float pi = 3.14159265f;

    click_sound_high_.resize(click_samples);
    for (uint32_t i = 0; i < click_samples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sample_rate_);
        float envelope = std::exp(-t * 200.0f);
        float frequency = 1200.0f;
        float sample = std::sin(2.0f * pi * frequency * t) * envelope;
        sample += 0.5f * std::sin(2.0f * pi * frequency * 2.0f * t) * envelope;
        sample += 0.3f * std::sin(2.0f * pi * frequency * 3.0f * t) * envelope;
        click_sound_high_[i] = sample * 0.5f;
    }

    click_sound_low_.resize(click_samples);
    for (uint32_t i = 0; i < click_samples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sample_rate_);
        float envelope = std::exp(-t * 250.0f);
        float frequency = 800.0f;
        float sample = std::sin(2.0f * pi * frequency * t) * envelope;
        sample += 0.3f * std::sin(2.0f * pi * frequency * 2.0f * t) * envelope;
        click_sound_low_[i] = sample * 0.35f;
    }
}

void AudioEngine::shutdown() {
    if (impl_->device_initialized) {
        ma_device_uninit(&impl_->device);
        impl_->device_initialized = false;
    }
}

bool AudioEngine::load_clip(const std::string& filepath) {
    auto new_clip = std::make_unique<AudioClip>();
    if (!new_clip->load(filepath)) {
        return false;
    }
    clip_ = std::move(new_clip);
    playhead_frame_ = 0;
    reset_clip_bounds();
    return true;
}

void AudioEngine::unload_clip() {
    stop();
    clip_.reset();
}

void AudioEngine::play() {
    is_playing_ = true;
}

void AudioEngine::pause() {
    is_playing_ = false;
}

void AudioEngine::stop() {
    is_playing_ = false;
    playhead_frame_ = 0;
}

void AudioEngine::set_playhead_seconds(double seconds) {
    uint64_t frame = static_cast<uint64_t>(seconds * sample_rate_);
    if (clip_ && clip_->is_loaded()) {
        uint64_t trimmed_duration = clip_end_frame() - clip_start_frame();
        playhead_frame_ = std::min(frame, trimmed_duration);
    } else {
        playhead_frame_ = frame;
    }
}

double AudioEngine::playhead_seconds() const {
    return static_cast<double>(playhead_frame_.load()) / static_cast<double>(sample_rate_);
}

void AudioEngine::set_bpm(double bpm) {
    if (bpm >= 1.0 && bpm <= 999.0) {
        bpm_ = bpm;
    }
}

void AudioEngine::advance_playhead(uint32_t frames) {
    uint64_t new_frame = playhead_frame_.load() + frames;

    if (clip_ && clip_->is_loaded()) {
        uint64_t trimmed_duration = clip_end_frame() - clip_start_frame();
        if (new_frame >= trimmed_duration) {
            new_frame = trimmed_duration;
            is_playing_ = false;
        }
    }

    playhead_frame_.store(new_frame);
}

void AudioEngine::set_clip_start_seconds(double seconds) {
    if (seconds >= 0.0) {
        clip_start_seconds_ = seconds;
        if (clip_ && clip_->is_loaded()) {
            uint64_t trimmed_duration = clip_end_frame() - clip_start_frame();
            if (playhead_frame_.load() > trimmed_duration) {
                playhead_frame_ = trimmed_duration;
            }
        }
    }
}

void AudioEngine::set_clip_end_seconds(double seconds) {
    if (seconds >= 0.0) {
        clip_end_seconds_ = seconds;
        if (clip_ && clip_->is_loaded()) {
            uint64_t trimmed_duration = clip_end_frame() - clip_start_frame();
            if (playhead_frame_.load() > trimmed_duration) {
                playhead_frame_ = trimmed_duration;
            }
        }
    }
}

void AudioEngine::reset_clip_bounds() {
    clip_start_seconds_ = 0.0;
    if (clip_ && clip_->is_loaded()) {
        clip_end_seconds_ = clip_->duration_seconds();
    } else {
        clip_end_seconds_ = 0.0;
    }
}

double AudioEngine::trimmed_duration_seconds() const {
    return clip_end_seconds_.load() - clip_start_seconds_.load();
}

uint64_t AudioEngine::clip_start_frame() const {
    return static_cast<uint64_t>(clip_start_seconds_.load() * sample_rate_);
}

uint64_t AudioEngine::clip_end_frame() const {
    double end_seconds = clip_end_seconds_.load();
    if (end_seconds <= 0.0 && clip_ && clip_->is_loaded()) {
        return clip_->total_frames();
    }
    return static_cast<uint64_t>(end_seconds * sample_rate_);
}

void AudioEngine::set_active_clips(std::vector<ClipAudioState> clips) {
    std::lock_guard<std::mutex> lock(clips_mutex_);
    active_clips_back_ = std::move(clips);
    clips_swap_pending_ = true;
}

void AudioEngine::swap_active_clips_if_pending() {
    if (clips_swap_pending_.load()) {
        std::lock_guard<std::mutex> lock(clips_mutex_);
        std::swap(active_clips_front_, active_clips_back_);
        clips_swap_pending_ = false;
    }
}

} // namespace furious
