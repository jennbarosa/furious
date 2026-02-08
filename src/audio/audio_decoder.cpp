#include "furious/audio/audio_decoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <vector>

namespace furious {

struct AudioDecoder::Impl {
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;

    int audio_stream_index = -1;
    double duration_seconds = 0.0;
    bool is_open = false;
};

AudioDecoder::AudioDecoder() : impl_(std::make_unique<Impl>()) {}

AudioDecoder::~AudioDecoder() {
    close();
}

AudioDecoder::AudioDecoder(AudioDecoder&&) noexcept = default;
AudioDecoder& AudioDecoder::operator=(AudioDecoder&&) noexcept = default;

std::expected<void, std::string> AudioDecoder::open(const std::string& filepath) {
    close();

    if (avformat_open_input(&impl_->format_ctx, filepath.c_str(), nullptr, nullptr) < 0) {
        return std::unexpected("Failed to open file");
    }

    if (avformat_find_stream_info(impl_->format_ctx, nullptr) < 0) {
        close();
        return std::unexpected("Failed to find stream info");
    }

    for (unsigned i = 0; i < impl_->format_ctx->nb_streams; ++i) {
        if (impl_->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            impl_->audio_stream_index = static_cast<int>(i);
            break;
        }
    }

    if (impl_->audio_stream_index < 0) {
        impl_->is_open = true;
        return {};
    }

    AVStream* audio_stream = impl_->format_ctx->streams[impl_->audio_stream_index];
    AVCodecParameters* codecpar = audio_stream->codecpar;

    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        close();
        return std::unexpected("Failed to find audio decoder");
    }

    impl_->codec_ctx = avcodec_alloc_context3(codec);
    if (!impl_->codec_ctx) {
        close();
        return std::unexpected("Failed to allocate codec context");
    }

    if (avcodec_parameters_to_context(impl_->codec_ctx, codecpar) < 0) {
        close();
        return std::unexpected("Failed to copy codec parameters");
    }

    if (avcodec_open2(impl_->codec_ctx, codec, nullptr) < 0) {
        close();
        return std::unexpected("Failed to open codec");
    }

    if (impl_->format_ctx->duration != AV_NOPTS_VALUE) {
        impl_->duration_seconds = static_cast<double>(impl_->format_ctx->duration) / AV_TIME_BASE;
    } else if (audio_stream->duration != AV_NOPTS_VALUE) {
        impl_->duration_seconds = static_cast<double>(audio_stream->duration) *
            av_q2d(audio_stream->time_base);
    }

    impl_->frame = av_frame_alloc();
    impl_->packet = av_packet_alloc();

    if (!impl_->frame || !impl_->packet) {
        close();
        return std::unexpected("Failed to allocate frame/packet");
    }

    impl_->is_open = true;
    return {};
}

void AudioDecoder::close() {
    if (impl_->swr_ctx) {
        swr_free(&impl_->swr_ctx);
    }
    if (impl_->packet) {
        av_packet_free(&impl_->packet);
    }
    if (impl_->frame) {
        av_frame_free(&impl_->frame);
    }
    if (impl_->codec_ctx) {
        avcodec_free_context(&impl_->codec_ctx);
    }
    if (impl_->format_ctx) {
        avformat_close_input(&impl_->format_ctx);
    }

    impl_->audio_stream_index = -1;
    impl_->duration_seconds = 0.0;
    impl_->is_open = false;
}

bool AudioDecoder::is_open() const {
    return impl_->is_open;
}

bool AudioDecoder::has_audio_stream() const {
    return impl_->audio_stream_index >= 0;
}

double AudioDecoder::duration_seconds() const {
    return impl_->duration_seconds;
}

std::expected<AudioBuffer, std::string> AudioDecoder::extract_all(uint32_t target_sample_rate,
                                                                   uint32_t target_channels) {
    if (!impl_->is_open) {
        return std::unexpected("Decoder not open");
    }

    if (impl_->audio_stream_index < 0) {
        return std::unexpected("No audio stream");
    }

    AVCodecContext* ctx = impl_->codec_ctx;

    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout, static_cast<int>(target_channels));

    int ret = swr_alloc_set_opts2(
        &impl_->swr_ctx,
        &out_ch_layout,
        AV_SAMPLE_FMT_FLT,
        static_cast<int>(target_sample_rate),
        &ctx->ch_layout,
        ctx->sample_fmt,
        ctx->sample_rate,
        0,
        nullptr
    );

    if (ret < 0 || !impl_->swr_ctx) {
        av_channel_layout_uninit(&out_ch_layout);
        return std::unexpected("Failed to allocate resampler");
    }

    if (swr_init(impl_->swr_ctx) < 0) {
        av_channel_layout_uninit(&out_ch_layout);
        return std::unexpected("Failed to initialize resampler");
    }

    std::vector<float> all_samples;
    all_samples.reserve(static_cast<size_t>(impl_->duration_seconds * target_sample_rate * target_channels));

    while (av_read_frame(impl_->format_ctx, impl_->packet) >= 0) {
        if (impl_->packet->stream_index != impl_->audio_stream_index) {
            av_packet_unref(impl_->packet);
            continue;
        }

        ret = avcodec_send_packet(impl_->codec_ctx, impl_->packet);
        av_packet_unref(impl_->packet);

        if (ret < 0) continue;

        while (avcodec_receive_frame(impl_->codec_ctx, impl_->frame) >= 0) {
            int out_samples = swr_get_out_samples(impl_->swr_ctx, impl_->frame->nb_samples);
            if (out_samples <= 0) {
                av_frame_unref(impl_->frame);
                continue;
            }

            std::vector<float> frame_buffer(static_cast<size_t>(out_samples * target_channels));
            uint8_t* out_buffer = reinterpret_cast<uint8_t*>(frame_buffer.data());

            int converted = swr_convert(
                impl_->swr_ctx,
                &out_buffer,
                out_samples,
                const_cast<const uint8_t**>(impl_->frame->extended_data),
                impl_->frame->nb_samples
            );

            if (converted > 0) {
                size_t sample_count = static_cast<size_t>(converted * target_channels);
                all_samples.insert(all_samples.end(),
                                   frame_buffer.begin(),
                                   frame_buffer.begin() + sample_count);
            }

            av_frame_unref(impl_->frame);
        }
    }

    avcodec_send_packet(impl_->codec_ctx, nullptr);
    while (avcodec_receive_frame(impl_->codec_ctx, impl_->frame) >= 0) {
        int out_samples = swr_get_out_samples(impl_->swr_ctx, impl_->frame->nb_samples);
        if (out_samples <= 0) {
            av_frame_unref(impl_->frame);
            continue;
        }

        std::vector<float> frame_buffer(static_cast<size_t>(out_samples * target_channels));
        uint8_t* out_buffer = reinterpret_cast<uint8_t*>(frame_buffer.data());

        int converted = swr_convert(
            impl_->swr_ctx,
            &out_buffer,
            out_samples,
            const_cast<const uint8_t**>(impl_->frame->extended_data),
            impl_->frame->nb_samples
        );

        if (converted > 0) {
            size_t sample_count = static_cast<size_t>(converted * target_channels);
            all_samples.insert(all_samples.end(),
                               frame_buffer.begin(),
                               frame_buffer.begin() + sample_count);
        }

        av_frame_unref(impl_->frame);
    }

    int flushed = swr_convert(impl_->swr_ctx, nullptr, 0, nullptr, 0);
    if (flushed > 0) {
        std::vector<float> flush_buffer(static_cast<size_t>(flushed * target_channels));
        uint8_t* out_buffer = reinterpret_cast<uint8_t*>(flush_buffer.data());
        int got = swr_convert(impl_->swr_ctx, &out_buffer, flushed, nullptr, 0);
        if (got > 0) {
            size_t sample_count = static_cast<size_t>(got * target_channels);
            all_samples.insert(all_samples.end(),
                               flush_buffer.begin(),
                               flush_buffer.begin() + sample_count);
        }
    }

    av_channel_layout_uninit(&out_ch_layout);

    return AudioBuffer(std::move(all_samples), target_sample_rate, target_channels);
}

}
