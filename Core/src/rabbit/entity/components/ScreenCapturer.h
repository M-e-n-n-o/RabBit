#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "graphics/RenderResource.h"
#include "graphics/Renderer.h"
#include "math/Vector.h"

namespace RB::Entity
{
    class ScreenCapturer : public ObjectComponent
    {
    public:
        ScreenCapturer();

        void Update() override;

        // Simply used to calculate the required readback size
        uint64_t PrepareCapture(Graphics::Texture2D* target);

        void Capture(void* readback_data);

        // Should only be called a frame after the screenshot has been scheduled!
        bool ReadCapture(bool should_block = true);

        // For the Renderer
        bool ShouldMakeCapture() const { return m_ScheduledGpuCapture; }
        Shared<Graphics::ReadbackBuffer> GetReadbackBuffer() const { return m_ReadbackBuffer; }

    private:
        bool                               m_ScheduledGpuCapture;
        bool                               m_ReadingCapture;
        uint64_t                           m_LastCapturedFrame;
        Shared<Graphics::ReadbackBuffer>   m_ReadbackBuffer;
        void*                              m_DestinationData;
    };
}