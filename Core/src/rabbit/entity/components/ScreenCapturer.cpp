#include "RabBitCommon.h"
#include "ScreenCapturer.h"

#include "app/Application.h"

namespace RB::Entity
{
    ScreenCapturer::ScreenCapturer()
        : m_ScheduledGpuCapture(false)
        , m_ReadingCapture(false)
        , m_LastCapturedFrame(0)
        , m_ReadbackBuffer(nullptr)
        , m_DestinationData(nullptr)
    {
    }

    void ScreenCapturer::Update()
    {
        uint64_t frame_idx = Application::GetInstance()->GetFrameIndex();

        if (frame_idx > m_LastCapturedFrame)
        {
            // The RenderThread should have picked up the capture request from last frame
            m_ScheduledGpuCapture = false;
        }

        if (m_ReadbackBuffer)
        {
            if (frame_idx > m_LastCapturedFrame + 10)
            {
                // Delete the readback buffer if we are not continuously capturing
                m_ReadbackBuffer.reset();
                m_ReadbackBuffer = nullptr;
            }
        }
    }

    uint64_t ScreenCapturer::PrepareCapture(Graphics::Texture2D* target)
    {
        if (m_ReadbackBuffer)
            return m_ReadbackBuffer->GetSize();

        m_ReadbackBuffer = Graphics::ReadbackBuffer::Create("ScreenCapture Readback", target);

        return m_ReadbackBuffer->GetSize();
    }

    void ScreenCapturer::Capture(void* readback_data)
    {
        uint64_t frame_idx = Application::GetInstance()->GetFrameIndex();

        if (m_LastCapturedFrame >= frame_idx)
            return; // Capture already scheduled

        // Make sure we are lock stepping with the RenderThread so we can not run ahead and let the RenderThread skip any frames!
        Application::GetInstance()->GetRenderer()->SyncRenderer();

        m_ScheduledGpuCapture   = true;
        m_ReadingCapture        = true;
        m_LastCapturedFrame     = frame_idx;
        m_DestinationData       = readback_data;
    }

    bool ScreenCapturer::ReadCapture(bool should_block)
    {
        if (!m_ReadingCapture)
            return false;

        // Keep lock stepping
        Application::GetInstance()->GetRenderer()->SyncRenderer();

        bool success = m_ReadbackBuffer->GetData(m_DestinationData, should_block);

        if (success)
            m_ReadingCapture = false;

        return success;
    }
}