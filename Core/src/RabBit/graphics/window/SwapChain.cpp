#include "RabBitPch.h"
#include "SwapChain.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/window/NativeWindow.h"

namespace RB::Graphics::Window
{
	SwapChain* g_SwapChain = nullptr;

	SwapChain::SwapChain(GPtr<ID3D12CommandQueue> command_queue, uint32_t width, uint32_t height, uint32_t buffer_count, 
		DXGI_FORMAT format)
		:	m_BackBufferCount(buffer_count),
			m_Width(width),
			m_Height(height)
	{
		RB_ASSERT_FATAL_RELEASE(g_GraphicsDevice->IsFormatSupported(format), "Device does not support passed in format, can not create SwapChain");

		GPtr<IDXGIFactory4> dxgi_factory;
		UINT factory_flags = 0;
#ifdef RB_CONFIG_DEBUG
		factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		RB_ASSERT_FATAL_RELEASE_D3D(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&dxgi_factory)), "Could not create factory");

		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
		swap_chain_desc.Width		= width;
		swap_chain_desc.Height		= height;
		swap_chain_desc.Format		= format;
		swap_chain_desc.Stereo		= FALSE;
		swap_chain_desc.SampleDesc	= { 1, 0 };
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.BufferCount = buffer_count;
		swap_chain_desc.Scaling		= DXGI_SCALING_STRETCH;
		swap_chain_desc.SwapEffect	= DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swap_chain_desc.AlphaMode	= DXGI_ALPHA_MODE_UNSPECIFIED;
		swap_chain_desc.Flags		= g_GraphicsDevice->IsFeatureSupported(DXGI_FEATURE_PRESENT_ALLOW_TEARING) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		GPtr<IDXGISwapChain1> swap_chain1;
		RB_ASSERT_FATAL_RELEASE_D3D(dxgi_factory->CreateSwapChainForHwnd(
			command_queue.Get(),
			g_NativeWindow->GetWindowHandle(),
			&swap_chain_desc,
			nullptr,
			nullptr,
			&swap_chain1
		), "Could not create swap chain");

		RB_ASSERT_FATAL_RELEASE_D3D(swap_chain1.As(&m_SwapChain), "Could not convert the swap chain to abstraction level 4");

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
	}
	
	SwapChain::~SwapChain()
	{

	}
}