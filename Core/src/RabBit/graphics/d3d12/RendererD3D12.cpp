#include "RabBitCommon.h"
#include "RendererD3D12.h"
#include "GraphicsDevice.h"
#include "resource/ResourceManager.h"
#include "resource/ResourceStateManager.h"
#include "shaders/ShaderSystem.h"
#include "pipeline/Pipeline.h"

namespace RB::Graphics::D3D12
{
	RendererD3D12::RendererD3D12()
	{
		g_GraphicsDevice		= new GraphicsDevice();
		g_ResourceStateManager	= new ResourceStateManager();
		g_ResourceManager		= new ResourceManager();
		g_ShaderSystem			= new ShaderSystem();
		g_PipelineManager		= new PipelineManager();
	}

	RendererD3D12::~RendererD3D12()
	{
		delete g_PipelineManager;
		delete g_ShaderSystem;
		delete g_ResourceStateManager;
		delete g_ResourceManager;
		delete g_GraphicsDevice;
	}

	void RendererD3D12::OnFrameStart()
	{
		g_ResourceManager->StartFrame();
	}

	void RendererD3D12::OnFrameEnd()
	{
		g_ResourceManager->EndFrame();
	}
}