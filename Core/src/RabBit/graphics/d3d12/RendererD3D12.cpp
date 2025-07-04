#include "RabBitCommon.h"
#include "RendererD3D12.h"
#include "GraphicsDevice.h"
#include "resource/ResourceManager.h"
#include "resource/ResourceStateManager.h"
#include "resource/Descriptor.h"
#include "ShaderSystem.h"
#include "Pipeline.h"

namespace RB::Graphics::D3D12
{
    RendererD3D12::RendererD3D12(bool enable_debug_layer)
        : Renderer(true)
    {
        g_GraphicsDevice        = new GraphicsDevice(enable_debug_layer);
        g_DescriptorManager     = new DescriptorManager();
        g_ResourceManager       = new ResourceManager();
        g_ResourceStateManager  = new ResourceStateManager();
        g_ShaderSystem          = new ShaderSystem();
        g_PipelineManager       = new PipelineManager();
    }

    RendererD3D12::~RendererD3D12()
    {
        delete g_PipelineManager;
        delete g_ShaderSystem;
        delete g_ResourceStateManager;
        delete g_ResourceManager;
        delete g_DescriptorManager;
        delete g_GraphicsDevice;
    }

    void RendererD3D12::OnFrameStart()
    {
    }

    void RendererD3D12::OnFrameEnd()
    {
        g_ResourceManager->UpdateBookkeeping();
        g_DescriptorManager->CycleDescriptors();
    }

    void RendererD3D12::SyncWithGpu()
    {
        g_GraphicsDevice->WaitUntilIdle();
        g_ResourceManager->UpdateBookkeeping();
    }
}