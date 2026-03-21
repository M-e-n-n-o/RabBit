#if RB_GRAPHICS_API_D3D12

#include "RabBitCommon.h"
#include "RendererD3D12.h"
#include "GraphicsDevice.h"
#include "graphics/ShaderSystem.h"
#include "resource/ResourceManager.h"
#include "resource/ResourceStateManager.h"
#include "resource/UploadAllocator.h"
#include "resource/Descriptor.h"
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
        g_TransientCBVAllocator = new TransientUploadBuffer("Transient CBV Allocation", k64KB);
        g_TransientVBAllocator  = new TransientUploadBuffer("Transient VB Allocation",  k64KB);
        g_PipelineManager       = new PipelineManager();
    }

    RendererD3D12::~RendererD3D12()
    {
        delete g_PipelineManager;
        delete g_TransientVBAllocator;
        delete g_TransientCBVAllocator;
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
        g_TransientCBVAllocator->CycleBuffers();
        g_TransientVBAllocator->CycleBuffers();
    }

    void RendererD3D12::SyncWithGpu()
    {
        g_GraphicsDevice->WaitUntilIdle();
        g_ResourceManager->FlushBookkeeping();
    }
}
#endif