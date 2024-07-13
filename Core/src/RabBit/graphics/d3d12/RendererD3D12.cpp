#include "RabBitCommon.h"
#include "RendererD3D12.h"
#include "GraphicsDevice.h"
#include "resource/ResourceManager.h"
#include "resource/ResourceStateManager.h"
#include "resource/BindlessDescriptorHeap.h"
#include "Pipeline.h"

namespace RB::Graphics::D3D12
{
	RendererD3D12::RendererD3D12(bool enable_debug_layer)
		: Renderer(true)
	{
		g_GraphicsDevice		= new GraphicsDevice(enable_debug_layer);
		g_BindlessSrvUavHeap	= new BindlessDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, BINDLESS_SRV_UAV_DESCRIPTOR_HEAP_SIZE);
		g_ResourceStateManager	= new ResourceStateManager();
		g_ResourceManager		= new ResourceManager();

		Init();

		g_PipelineManager		= new PipelineManager(m_ShaderSystem);
	}

	RendererD3D12::~RendererD3D12()
	{
		delete g_PipelineManager;
		delete g_ResourceStateManager;
		delete g_ResourceManager;
		delete g_BindlessSrvUavHeap;
		delete g_GraphicsDevice;
	}

	void RendererD3D12::OnFrameStart()
	{

	}

	void RendererD3D12::OnFrameEnd()
	{
		g_ResourceManager->UpdateBookkeeping();
	}
	
	void RendererD3D12::SyncWithGpu()
	{
		g_GraphicsDevice->WaitUntilIdle();
		g_ResourceManager->UpdateBookkeeping();
	}
}