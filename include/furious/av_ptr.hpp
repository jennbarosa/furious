#pragma once

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

namespace furious {

struct AVFormatContextCloser {
    void operator()(AVFormatContext* p) const { avformat_close_input(&p); }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* p) const { avcodec_free_context(&p); }
};

struct AVBufferRefDeleter {
    void operator()(AVBufferRef* p) const { av_buffer_unref(&p); }
};

struct SwsContextDeleter {
    void operator()(SwsContext* p) const { sws_freeContext(p); }
};

struct SwrContextDeleter {
    void operator()(SwrContext* p) const { swr_free(&p); }
};

struct AVFrameDeleter {
    void operator()(AVFrame* p) const { av_frame_free(&p); }
};

struct AVPacketDeleter {
    void operator()(AVPacket* p) const { av_packet_free(&p); }
};

using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextCloser>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVBufferRefPtr = std::unique_ptr<AVBufferRef, AVBufferRefDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
using SwrContextPtr = std::unique_ptr<SwrContext, SwrContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;

} // namespace furious
