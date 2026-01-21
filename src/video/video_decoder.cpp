#include "furious/video/video_decoder.hpp"

#include <algorithm>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>
}

namespace furious {

constexpr int MAX_PREVIEW_WIDTH = 1280;
constexpr int MAX_PREVIEW_HEIGHT = 720;

struct VideoDecoder::Impl {
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVBufferRef* hw_device_ctx = nullptr;  
    SwsContext* sws_ctx = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* sw_frame = nullptr;  
    AVFrame* frame_rgba = nullptr;
    AVPacket* packet = nullptr;

    int video_stream_index = -1;
    int source_width = 0;   
    int source_height = 0;
    int width = 0;          
    int height = 0;
    int pix_fmt = -1;  
    AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;  
    double fps = 30.0;
    double duration_seconds = 0.0;
    int64_t total_frames = 0;

    bool is_open = false;
    bool using_hw_decode = false;
    std::string decoder_name = "None";

    double last_decoded_pts = -1.0;
    bool has_buffered_frame = false;  
};

VideoDecoder::VideoDecoder() : impl_(std::make_unique<Impl>()) {}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(const std::string& filepath) {
    close();

    if (avformat_open_input(&impl_->format_ctx, filepath.c_str(), nullptr, nullptr) < 0) {
        return false;
    }

    if (avformat_find_stream_info(impl_->format_ctx, nullptr) < 0) {
        close();
        return false;
    }

    for (unsigned i = 0; i < impl_->format_ctx->nb_streams; ++i) {
        if (impl_->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            impl_->video_stream_index = static_cast<int>(i);
            break;
        }
    }

    if (impl_->video_stream_index < 0) {
        close();
        return false;
    }

    AVStream* video_stream = impl_->format_ctx->streams[impl_->video_stream_index];
    AVCodecParameters* codecpar = video_stream->codecpar;

    const AVHWDeviceType hw_types[] = {
        AV_HWDEVICE_TYPE_VAAPI,
        AV_HWDEVICE_TYPE_CUDA,
        AV_HWDEVICE_TYPE_VDPAU,
        AV_HWDEVICE_TYPE_NONE
    };

    const AVCodec* codec = nullptr;

    void* iter = nullptr;
    while ((codec = av_codec_iterate(&iter)) != nullptr) {
        if (!av_codec_is_decoder(codec)) continue;
        if (codec->id != codecpar->codec_id) continue;

        for (int i = 0; hw_types[i] != AV_HWDEVICE_TYPE_NONE; ++i) {
            for (int j = 0;; ++j) {
                const AVCodecHWConfig* config = avcodec_get_hw_config(codec, j);
                if (!config) break;

                if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                    config->device_type == hw_types[i]) {
                    if (av_hwdevice_ctx_create(&impl_->hw_device_ctx, hw_types[i],
                                               nullptr, nullptr, 0) >= 0) {
                        impl_->codec_ctx = avcodec_alloc_context3(codec);
                        if (!impl_->codec_ctx) {
                            av_buffer_unref(&impl_->hw_device_ctx);
                            continue;
                        }

                        if (avcodec_parameters_to_context(impl_->codec_ctx, codecpar) < 0) {
                            avcodec_free_context(&impl_->codec_ctx);
                            av_buffer_unref(&impl_->hw_device_ctx);
                            continue;
                        }

                        impl_->codec_ctx->hw_device_ctx = av_buffer_ref(impl_->hw_device_ctx);
                        impl_->hw_pix_fmt = config->pix_fmt;

                        if (avcodec_open2(impl_->codec_ctx, codec, nullptr) >= 0) {
                            impl_->using_hw_decode = true;
                            impl_->decoder_name = std::string(codec->name) + " + " +
                                                  av_hwdevice_get_type_name(hw_types[i]) + " (HW)";
                            goto decoder_ready;
                        }

                        avcodec_free_context(&impl_->codec_ctx);
                        av_buffer_unref(&impl_->hw_device_ctx);
                    }
                }
            }
        }
    }

    codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        close();
        return false;
    }

    impl_->codec_ctx = avcodec_alloc_context3(codec);
    if (!impl_->codec_ctx) {
        close();
        return false;
    }

    if (avcodec_parameters_to_context(impl_->codec_ctx, codecpar) < 0) {
        close();
        return false;
    }

    if (avcodec_open2(impl_->codec_ctx, codec, nullptr) < 0) {
        close();
        return false;
    }

    impl_->decoder_name = std::string(codec->name) + " (SW)";

decoder_ready:

    impl_->source_width = impl_->codec_ctx->width;
    impl_->source_height = impl_->codec_ctx->height;
    impl_->pix_fmt = impl_->codec_ctx->pix_fmt;

    if (impl_->source_width <= 0 || impl_->source_height <= 0 || impl_->pix_fmt < 0) {
        close();
        return false;
    }

    impl_->width = impl_->source_width;
    impl_->height = impl_->source_height;

    if (impl_->width > MAX_PREVIEW_WIDTH || impl_->height > MAX_PREVIEW_HEIGHT) {
        double scale_w = static_cast<double>(MAX_PREVIEW_WIDTH) / impl_->source_width;
        double scale_h = static_cast<double>(MAX_PREVIEW_HEIGHT) / impl_->source_height;
        double scale = std::min(scale_w, scale_h);

        impl_->width = static_cast<int>(impl_->source_width * scale);
        impl_->height = static_cast<int>(impl_->source_height * scale);

        // ensure dimensions are even (required by some codecs)
        impl_->width &= ~1;
        impl_->height &= ~1;
    }

    if (video_stream->avg_frame_rate.den != 0 && video_stream->avg_frame_rate.num != 0) {
        impl_->fps = av_q2d(video_stream->avg_frame_rate);
    } else if (video_stream->r_frame_rate.den != 0 && video_stream->r_frame_rate.num != 0) {
        impl_->fps = av_q2d(video_stream->r_frame_rate);
    }

    if (impl_->format_ctx->duration != AV_NOPTS_VALUE) {
        impl_->duration_seconds = static_cast<double>(impl_->format_ctx->duration) / AV_TIME_BASE;
    } else if (video_stream->duration != AV_NOPTS_VALUE) {
        impl_->duration_seconds = static_cast<double>(video_stream->duration) *
            av_q2d(video_stream->time_base);
    }

    impl_->total_frames = static_cast<int64_t>(impl_->duration_seconds * impl_->fps);

    impl_->sws_ctx = sws_getContext(
        impl_->source_width, impl_->source_height, static_cast<AVPixelFormat>(impl_->pix_fmt),
        impl_->width, impl_->height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!impl_->sws_ctx) {
        close();
        return false;
    }

    impl_->frame = av_frame_alloc();
    impl_->sw_frame = av_frame_alloc();  
    impl_->frame_rgba = av_frame_alloc();
    impl_->packet = av_packet_alloc();

    if (!impl_->frame || !impl_->sw_frame || !impl_->frame_rgba || !impl_->packet) {
        close();
        return false;
    }

    impl_->is_open = true;
    return true;
}

void VideoDecoder::close() {
    if (impl_->packet) {
        av_packet_free(&impl_->packet);
    }
    if (impl_->frame) {
        av_frame_free(&impl_->frame);
    }
    if (impl_->sw_frame) {
        av_frame_free(&impl_->sw_frame);
    }
    if (impl_->frame_rgba) {
        av_frame_free(&impl_->frame_rgba);
    }
    if (impl_->sws_ctx) {
        sws_freeContext(impl_->sws_ctx);
        impl_->sws_ctx = nullptr;
    }
    if (impl_->codec_ctx) {
        avcodec_free_context(&impl_->codec_ctx);
    }
    if (impl_->hw_device_ctx) {
        av_buffer_unref(&impl_->hw_device_ctx);
    }
    if (impl_->format_ctx) {
        avformat_close_input(&impl_->format_ctx);
    }

    impl_->video_stream_index = -1;
    impl_->source_width = 0;
    impl_->source_height = 0;
    impl_->width = 0;
    impl_->height = 0;
    impl_->pix_fmt = -1;
    impl_->hw_pix_fmt = AV_PIX_FMT_NONE;
    impl_->fps = 30.0;
    impl_->duration_seconds = 0.0;
    impl_->total_frames = 0;
    impl_->is_open = false;
    impl_->using_hw_decode = false;
    impl_->decoder_name = "None";
    impl_->last_decoded_pts = -1.0;
}

bool VideoDecoder::is_open() const {
    return impl_->is_open;
}

bool VideoDecoder::seek_and_decode(double timestamp_seconds, std::vector<uint8_t>& rgba_buffer) {
    if (!impl_->is_open) return false;

    if (!impl_->format_ctx || !impl_->codec_ctx || !impl_->frame || !impl_->packet || !impl_->sws_ctx) {
        return false;
    }

    if (impl_->video_stream_index < 0 ||
        static_cast<unsigned>(impl_->video_stream_index) >= impl_->format_ctx->nb_streams) {
        return false;
    }

    if (timestamp_seconds < 0.0) timestamp_seconds = 0.0;

    AVStream* video_stream = impl_->format_ctx->streams[impl_->video_stream_index];
    if (!video_stream || !video_stream->time_base.den) return false;

    double frame_duration = 1.0 / impl_->fps;

    bool need_seek = (impl_->last_decoded_pts < 0.0) ||
                     (timestamp_seconds < impl_->last_decoded_pts - frame_duration) ||
                     (timestamp_seconds > impl_->last_decoded_pts + 2.0);

    if (need_seek) {
        int64_t target_ts = static_cast<int64_t>(timestamp_seconds / av_q2d(video_stream->time_base));

        if (av_seek_frame(impl_->format_ctx, impl_->video_stream_index, target_ts,
                          AVSEEK_FLAG_BACKWARD) < 0) {
            return false;
        }

        avcodec_flush_buffers(impl_->codec_ctx);
        impl_->last_decoded_pts = -1.0;
    }

    bool tried_seek_fallback = need_seek; 

retry_decode:
    int read_result;
    while ((read_result = av_read_frame(impl_->format_ctx, impl_->packet)) >= 0) {
        if (impl_->packet->stream_index != impl_->video_stream_index) {
            av_packet_unref(impl_->packet);
            continue;
        }

        int ret = avcodec_send_packet(impl_->codec_ctx, impl_->packet);
        av_packet_unref(impl_->packet);

        if (ret < 0) continue;

        while (avcodec_receive_frame(impl_->codec_ctx, impl_->frame) >= 0) {
            double frame_ts = static_cast<double>(impl_->frame->pts) *
                av_q2d(video_stream->time_base);

            if (frame_ts >= timestamp_seconds - (frame_duration * 0.5)) {
                AVFrame* src_frame = impl_->frame;
                if (impl_->using_hw_decode && impl_->frame->format == impl_->hw_pix_fmt) {
                    if (av_hwframe_transfer_data(impl_->sw_frame, impl_->frame, 0) < 0) {
                        av_frame_unref(impl_->frame);
                        return false;
                    }
                    impl_->sw_frame->pts = impl_->frame->pts;
                    src_frame = impl_->sw_frame;
                }

                bool needs_sws_recreate = (src_frame->width != impl_->source_width) ||
                                          (src_frame->height != impl_->source_height) ||
                                          (src_frame->format != impl_->pix_fmt);

                if (needs_sws_recreate) {
                    if (impl_->sws_ctx) {
                        sws_freeContext(impl_->sws_ctx);
                        impl_->sws_ctx = nullptr;
                    }
                    impl_->source_width = src_frame->width;
                    impl_->source_height = src_frame->height;
                    impl_->pix_fmt = src_frame->format;

                    if (impl_->source_width <= 0 || impl_->source_height <= 0 || impl_->pix_fmt < 0) {
                        av_frame_unref(impl_->sw_frame);
                        av_frame_unref(impl_->frame);
                        return false;
                    }

                    impl_->width = impl_->source_width;
                    impl_->height = impl_->source_height;
                    if (impl_->width > MAX_PREVIEW_WIDTH || impl_->height > MAX_PREVIEW_HEIGHT) {
                        double scale_w = static_cast<double>(MAX_PREVIEW_WIDTH) / impl_->source_width;
                        double scale_h = static_cast<double>(MAX_PREVIEW_HEIGHT) / impl_->source_height;
                        double scale = std::min(scale_w, scale_h);
                        impl_->width = static_cast<int>(impl_->source_width * scale) & ~1;
                        impl_->height = static_cast<int>(impl_->source_height * scale) & ~1;
                    }

                    impl_->sws_ctx = sws_getContext(
                        impl_->source_width, impl_->source_height, static_cast<AVPixelFormat>(impl_->pix_fmt),
                        impl_->width, impl_->height, AV_PIX_FMT_RGBA,
                        SWS_BILINEAR, nullptr, nullptr, nullptr
                    );
                    if (!impl_->sws_ctx) {
                        av_frame_unref(impl_->sw_frame);
                        av_frame_unref(impl_->frame);
                        return false;
                    }
                }

                if (impl_->width <= 0 || impl_->height <= 0 || !impl_->sws_ctx) {
                    av_frame_unref(impl_->sw_frame);
                    av_frame_unref(impl_->frame);
                    return false;
                }

                if (impl_->width > 8192 || impl_->height > 8192) {
                    av_frame_unref(impl_->sw_frame);
                    av_frame_unref(impl_->frame);
                    return false;
                }

                if (!src_frame->data[0]) {
                    av_frame_unref(impl_->sw_frame);
                    av_frame_unref(impl_->frame);
                    return false;
                }

                size_t buffer_size = static_cast<size_t>(impl_->width) * static_cast<size_t>(impl_->height) * 4;
                rgba_buffer.resize(buffer_size);

                uint8_t* dest[1] = { rgba_buffer.data() };
                int dest_linesize[1] = { impl_->width * 4 };

                sws_scale(impl_->sws_ctx,
                    src_frame->data, src_frame->linesize,
                    0, impl_->source_height,
                    dest, dest_linesize);

                impl_->last_decoded_pts = frame_ts;

                av_frame_unref(impl_->sw_frame);
                av_frame_unref(impl_->frame);
                return true;
            } else {
                av_frame_unref(impl_->frame);
            }
        }
    }

    if (read_result == AVERROR_EOF) {
        avcodec_send_packet(impl_->codec_ctx, nullptr);

        while (avcodec_receive_frame(impl_->codec_ctx, impl_->frame) >= 0) {
            double frame_ts = static_cast<double>(impl_->frame->pts) *
                av_q2d(video_stream->time_base);

            if (frame_ts >= timestamp_seconds - (frame_duration * 0.5)) {
                AVFrame* src_frame = impl_->frame;
                if (impl_->using_hw_decode && impl_->frame->format == impl_->hw_pix_fmt) {
                    if (av_hwframe_transfer_data(impl_->sw_frame, impl_->frame, 0) < 0) {
                        av_frame_unref(impl_->frame);
                        continue;
                    }
                    impl_->sw_frame->pts = impl_->frame->pts;
                    src_frame = impl_->sw_frame;
                }

                if (impl_->width <= 0 || impl_->height <= 0 || !impl_->sws_ctx) {
                    av_frame_unref(impl_->sw_frame);
                    av_frame_unref(impl_->frame);
                    continue;
                }

                if (!src_frame->data[0]) {
                    av_frame_unref(impl_->sw_frame);
                    av_frame_unref(impl_->frame);
                    continue;
                }

                size_t buffer_size = static_cast<size_t>(impl_->width) * static_cast<size_t>(impl_->height) * 4;
                rgba_buffer.resize(buffer_size);

                uint8_t* dest[1] = { rgba_buffer.data() };
                int dest_linesize[1] = { impl_->width * 4 };

                sws_scale(impl_->sws_ctx,
                    src_frame->data, src_frame->linesize,
                    0, impl_->source_height,
                    dest, dest_linesize);

                impl_->last_decoded_pts = frame_ts;

                av_frame_unref(impl_->sw_frame);
                av_frame_unref(impl_->frame);
                return true;
            }
            av_frame_unref(impl_->frame);
        }
    }

    if (!tried_seek_fallback) {
        tried_seek_fallback = true;
        int64_t target_ts = static_cast<int64_t>(timestamp_seconds / av_q2d(video_stream->time_base));

        if (av_seek_frame(impl_->format_ctx, impl_->video_stream_index, target_ts,
                          AVSEEK_FLAG_BACKWARD) >= 0) {
            avcodec_flush_buffers(impl_->codec_ctx);
            impl_->last_decoded_pts = -1.0;
            goto retry_decode;
        }
    }

    return false;
}

bool VideoDecoder::decode_next_frame(std::vector<uint8_t>& rgba_buffer) {
    if (!impl_->is_open) return false;

    while (av_read_frame(impl_->format_ctx, impl_->packet) >= 0) {
        if (impl_->packet->stream_index != impl_->video_stream_index) {
            av_packet_unref(impl_->packet);
            continue;
        }

        int ret = avcodec_send_packet(impl_->codec_ctx, impl_->packet);
        av_packet_unref(impl_->packet);

        if (ret < 0) continue;

        if (avcodec_receive_frame(impl_->codec_ctx, impl_->frame) >= 0) {
            bool needs_sws_recreate = (impl_->frame->width != impl_->source_width) ||
                                      (impl_->frame->height != impl_->source_height) ||
                                      (impl_->frame->format != impl_->pix_fmt);

            if (needs_sws_recreate) {
                if (impl_->sws_ctx) {
                    sws_freeContext(impl_->sws_ctx);
                    impl_->sws_ctx = nullptr;
                }
                impl_->source_width = impl_->frame->width;
                impl_->source_height = impl_->frame->height;
                impl_->pix_fmt = impl_->frame->format;

                if (impl_->source_width <= 0 || impl_->source_height <= 0 || impl_->pix_fmt < 0) {
                    av_frame_unref(impl_->frame);
                    return false;
                }

                impl_->width = impl_->source_width;
                impl_->height = impl_->source_height;
                if (impl_->width > MAX_PREVIEW_WIDTH || impl_->height > MAX_PREVIEW_HEIGHT) {
                    double scale_w = static_cast<double>(MAX_PREVIEW_WIDTH) / impl_->source_width;
                    double scale_h = static_cast<double>(MAX_PREVIEW_HEIGHT) / impl_->source_height;
                    double scale = std::min(scale_w, scale_h);
                    impl_->width = static_cast<int>(impl_->source_width * scale) & ~1;
                    impl_->height = static_cast<int>(impl_->source_height * scale) & ~1;
                }

                impl_->sws_ctx = sws_getContext(
                    impl_->source_width, impl_->source_height, static_cast<AVPixelFormat>(impl_->pix_fmt),
                    impl_->width, impl_->height, AV_PIX_FMT_RGBA,
                    SWS_BILINEAR, nullptr, nullptr, nullptr
                );
                if (!impl_->sws_ctx) {
                    av_frame_unref(impl_->frame);
                    return false;
                }
            }

            if (impl_->width <= 0 || impl_->height <= 0 || !impl_->sws_ctx) {
                av_frame_unref(impl_->frame);
                return false;
            }

            if (impl_->width > 8192 || impl_->height > 8192) {
                av_frame_unref(impl_->frame);
                return false;
            }

            if (!impl_->frame->data[0]) {
                av_frame_unref(impl_->frame);
                return false;
            }

            size_t buffer_size = static_cast<size_t>(impl_->width) * static_cast<size_t>(impl_->height) * 4;
            rgba_buffer.resize(buffer_size);

            uint8_t* dest[1] = { rgba_buffer.data() };
            int dest_linesize[1] = { impl_->width * 4 };

            sws_scale(impl_->sws_ctx,
                impl_->frame->data, impl_->frame->linesize,
                0, impl_->height,
                dest, dest_linesize);

            av_frame_unref(impl_->frame);
            return true;
        }
    }

    return false;
}

int VideoDecoder::width() const { return impl_->width; }
int VideoDecoder::height() const { return impl_->height; }
double VideoDecoder::fps() const { return impl_->fps; }
double VideoDecoder::duration_seconds() const { return impl_->duration_seconds; }
int64_t VideoDecoder::total_frames() const { return impl_->total_frames; }
std::string VideoDecoder::decoder_type() const { return impl_->decoder_name; }

} // namespace furious
