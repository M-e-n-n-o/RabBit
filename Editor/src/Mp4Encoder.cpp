#include "Mp4Encoder.h"
#include "Utils.h"

extern "C" 
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

using namespace RB::Graphics;

namespace Editor
{
    #define VIDEO_ENCODER       AV_CODEC_ID_H264
    #define VIDEO_PIXEL_FORMAT  AV_PIX_FMT_YUV420P

    AVPixelFormat ToAVFormat(RenderResourceFormat format)
    {
        switch (format)
        {
        case RB::Graphics::RenderResourceFormat::R8G8B8A8_SRGB:
        case RB::Graphics::RenderResourceFormat::B8G8R8A8_UNORM:
            return AV_PIX_FMT_BGRA;
        case RB::Graphics::RenderResourceFormat::R8G8B8A8_UNORM:
            return AV_PIX_FMT_RGBA;
        default:
            RB_LOG_ERROR("Cannot convert engine format to AVPixelFormat");
            return AV_PIX_FMT_NONE;
        }
    }

    void LogCallback(void*, int level, const char* fmt, va_list vl)
    {
        std::string text = FormatToString(fmt, vl);

        if (level == AV_LOG_WARNING)
        {
            RB_LOG_WARN("MP4 Encoder warning: %s", text.c_str());
        }
        else if (level < AV_LOG_WARNING)
        {
            RB_LOG_ERROR("MP4 Encoder error: %s", text.c_str());
        }
    }

    Mp4Encoder::Mp4Encoder(const char* file_name, uint32_t width, uint32_t height, uint32_t fps, uint64_t bitrate, RenderResourceFormat format)
        : m_IsEncoding(false)
        , m_FormatContext(nullptr)
        , m_CodecContext(nullptr)
        , m_Stream(nullptr)
        , m_SwsContext(nullptr)
        , m_Width(width)
        , m_Height(height)
        , m_FrameIndex(0)
    {
        RB_LOG("Starting with the encoding of: %s", file_name);

        m_FormatStride = GetElementSizeFromFormat(format);

        av_log_set_level(AV_LOG_WARNING);
        av_log_set_callback(LogCallback);

        avformat_alloc_output_context2(&m_FormatContext, nullptr, nullptr, file_name);

        m_Stream = avformat_new_stream(m_FormatContext, nullptr);

        const AVCodec* codec = avcodec_find_encoder(VIDEO_ENCODER);

        m_CodecContext = avcodec_alloc_context3(codec);
        m_CodecContext->width       = m_Width;
        m_CodecContext->height      = m_Height;
        m_CodecContext->time_base   = { 1, (int)fps };
        m_CodecContext->framerate   = { (int)fps, 1 };
        m_CodecContext->pix_fmt     = VIDEO_PIXEL_FORMAT;
        m_CodecContext->bit_rate    = (int64_t)bitrate;

        if (m_FormatContext->oformat->flags & AVFMT_GLOBALHEADER)
            m_CodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        int ret = avcodec_open2(m_CodecContext, codec, nullptr);
        if (ret < 0)
        {
            char err[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err, sizeof(err));
            RB_LOG_ERROR("Failed to open encoder: %s", err);
            return;
        }

        avcodec_parameters_from_context(m_Stream->codecpar, m_CodecContext);

        m_Stream->time_base = m_CodecContext->time_base;

        avio_open(&m_FormatContext->pb, file_name, AVIO_FLAG_WRITE);
        avformat_write_header(m_FormatContext, nullptr);

        m_SwsContext = sws_getContext(m_Width, m_Height, ToAVFormat(format),
                                      m_Width, m_Height, VIDEO_PIXEL_FORMAT,
                                      SWS_BILINEAR, nullptr, nullptr,
                                      nullptr);

        m_IsEncoding = true;
    }

    Mp4Encoder::~Mp4Encoder()
    {
        RB_ASSERT_FATAL(m_IsEncoding == false, "MP4 encoding should be stopped before destroying the Mp4Encoder object");
    }

    void Mp4Encoder::Finish()
    {
        if (!m_IsEncoding)
        {
            RB_LOG_WARN("MP4 encoding has already been finished");
            return;
        }
        
        m_IsEncoding = false;

        Encode(nullptr);
        
        av_write_trailer(m_FormatContext);

        if (!(m_FormatContext->oformat->flags & AVFMT_NOFILE))
            avio_closep(&m_FormatContext->pb);

        sws_freeContext(m_SwsContext);
        avcodec_free_context(&m_CodecContext);
        avformat_free_context(m_FormatContext);

        RB_LOG("Finished MP4 encoding");
    }

    void Mp4Encoder::AddFrame(void* frame_data)
    {
        if (!m_IsEncoding)
            return;

        AVFrame* frame = av_frame_alloc();
        frame->format   = VIDEO_PIXEL_FORMAT;
        frame->width    = m_Width;
        frame->height   = m_Height;
        frame->pts      = m_FrameIndex++;

        av_frame_get_buffer(frame, 32);

        const uint8_t* slice[1] = { (uint8_t*)frame_data };
        const int stride[1]     = { m_Width * m_FormatStride };

        sws_scale(m_SwsContext,
                  slice,
                  stride,
                  0,
                  m_Height,
                  frame->data,
                  frame->linesize);

        Encode(frame);
        av_frame_free(&frame);
    }

    void Mp4Encoder::Encode(AVFrame* frame)
    {
        int ret = avcodec_send_frame(m_CodecContext, frame);
        if (ret < 0)
        {
            char err[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, err, sizeof(err));
            RB_LOG_ERROR("Encountered an error while encoding a frame: %s", err);
        }

        AVPacket* packet = av_packet_alloc();

        while (avcodec_receive_packet(m_CodecContext, packet) == 0)
        {
            packet->stream_index = m_Stream->index;
            
            av_packet_rescale_ts(packet, m_CodecContext->time_base, m_Stream->time_base);
            av_interleaved_write_frame(m_FormatContext, packet);
            av_packet_unref(packet);
        }

        av_packet_free(&packet);
    }
}