#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "RendererVK.h"
#include "GraphicsDevice.h"
#include "ResourceStateManager.h"
#include "ResourceLifetimeManager.h"

namespace RB::Graphics::VK
{
    RendererVK::RendererVK(bool enable_debug_layer)
        : Renderer(true)
    {
        g_GraphicsDevice          = new GraphicsDevice(enable_debug_layer);
        g_ResourceLifetimeManager = new ResourceLifetimeManager();
        g_ResourceStateManager    = new ResourceStateManager();
    }

    RendererVK::~RendererVK()
    {
        delete g_ResourceStateManager;
        delete g_ResourceLifetimeManager;
        delete g_GraphicsDevice;
    }

    void RendererVK::OnFrameStart()
    {
    }

    void RendererVK::OnFrameEnd()
    {
        g_ResourceLifetimeManager->Update();
    }

    void RendererVK::SyncWithGpu()
    {
        g_GraphicsDevice->WaitUntilIdle();
        g_ResourceLifetimeManager->Flush();
    }
}
#endif