#if RB_GRAPHICS_API_VULKAN

#pragma once
#include "RabBitCommon.h"
#include "graphics/Renderer.h"
#include "graphics/Window.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    #define TRANSIENT_CYCLES (BACK_BUFFER_COUNT + 1)

    class RendererVK : public Renderer
    {
    public:
        RendererVK(bool enable_validation_layer);
        ~RendererVK();

        void OnFrameStart() override;
        void OnFrameEnd() override;

        void SyncWithGpu() override;
    };
}
#endif