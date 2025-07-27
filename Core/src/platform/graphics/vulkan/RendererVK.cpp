#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "RendererVK.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::VK
{
    RendererVK::RendererVK(bool enable_debug_layer)
        : Renderer(true)
    {
        g_GraphicsDevice = new GraphicsDevice(enable_debug_layer);
    }

    RendererVK::~RendererVK()
    {
        delete g_GraphicsDevice;
    }

    void RendererVK::OnFrameStart()
    {
    }

    void RendererVK::OnFrameEnd()
    {
    }

    void RendererVK::SyncWithGpu()
    {
    }
}
#endif