#pragma once

#include <RabBit.h>
#include <cstdint>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct SwsContext;

namespace Editor
{
    class Mp4Encoder
    {
    public:
        Mp4Encoder(const char* file_name, uint32_t width, uint32_t height, uint32_t fps, uint64_t bitrate, RB::Graphics::RenderResourceFormat format);
        ~Mp4Encoder();

        bool IsValid() { return m_IsEncoding; }

        // Make sure that the input data is in sRGB space!
        void AddFrame(void* frame_data);

        void Finish();

    private:
        void Encode(AVFrame* frame);

        bool                m_IsEncoding;

        AVFormatContext*    m_FormatContext;
        AVCodecContext*     m_CodecContext;
        AVStream*           m_Stream;
        SwsContext*         m_SwsContext;

        uint32_t            m_Width;
        uint32_t            m_Height;
        uint32_t            m_FormatStride;
        uint64_t            m_FrameIndex;
    };
}