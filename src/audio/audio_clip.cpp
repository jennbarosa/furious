#include "furious/audio/audio_clip.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace furious {

bool AudioClip::load(const std::string& filepath) {
    unload();

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);
    ma_decoder decoder;

    if (ma_decoder_init_file(filepath.c_str(), &config, &decoder) != MA_SUCCESS) {
        return false;
    }

    ma_uint64 frame_count;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count) != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        return false;
    }

    sample_rate_ = decoder.outputSampleRate;
    channels_ = decoder.outputChannels;
    total_frames_ = frame_count;

    samples_.resize(frame_count * channels_);

    ma_uint64 frames_read;
    if (ma_decoder_read_pcm_frames(&decoder, samples_.data(), frame_count, &frames_read) != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        unload();
        return false;
    }

    ma_decoder_uninit(&decoder);
    filepath_ = filepath;
    return true;
}

void AudioClip::unload() {
    samples_.clear();
    filepath_.clear();
    sample_rate_ = 0;
    channels_ = 0;
    total_frames_ = 0;
}

double AudioClip::duration_seconds() const {
    if (sample_rate_ == 0) return 0.0;
    return static_cast<double>(total_frames_) / static_cast<double>(sample_rate_);
}

} // namespace furious
