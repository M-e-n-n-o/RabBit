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

		m_IsTearingSupported = g_GraphicsDevice->IsFeatureSupported(DXGI_FEATURE_PRESENT_ALLOW_TEARING);

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
		swap_chain_desc.Flags		= m_IsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		GPtr<IDXGISwapChain1> swap_chain1;
		RB_ASSERT_FATAL_RELEASE_D3D(dxgi_factory->CreateSwapChainForHwnd(
			command_queue.Get(),
			g_NativeWindow->GetWindowHandle(),
			&swap_chain_desc,
			nullptr,
			nullptr,
			&swap_chain1
		), "Could not create swap chain");

		RB_ASSERT_FATAL_RELEASE_D3D(swap_chain1.As(&m_NativeSwapChain), "Could not convert the swap chain to abstraction level 4");

		m_CurrentBackBufferIndex = m_NativeSwapChain->GetCurrentBackBufferIndex();

		CreateDescriptorHeap();
		m_DescriptorIncrementSize = g_GraphicsDevice->Get2()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		m_BackBuffers = new GPtr<ID3D12Resource>[m_BackBufferCount];
		UpdateRenderTargetViews();
	}
	
	SwapChain::~SwapChain()
	{
		delete[] m_BackBuffers;
	}

	void SwapChain::Present(bool use_vsync, bool use_tearing_if_supported)
	{
		UINT sync_interval = use_vsync ? 1 : 0;
		UINT present_flags = (m_IsTearingSupported && use_tearing_if_supported && !use_vsync) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		RB_ASSERT_FATAL_RELEASE_D3D(m_NativeSwapChain->Present(sync_interval, present_flags), "Failed to present frame");

		m_CurrentBackBufferIndex = m_NativeSwapChain->GetCurrentBackBufferIndex();
	}

	void SwapChain::Resize(const uint32_t width, const uint32_t height)
	{
		m_Width = width;
		m_Height = height;

		for (uint32_t back_buffer_index = 0; back_buffer_index < m_BackBufferCount; ++back_buffer_index)
		{
			m_BackBuffers[back_buffer_index].Reset();
		}

		DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
		RB_ASSERT_FATAL_RELEASE_D3D(m_NativeSwapChain->GetDesc(&swap_chain_desc), "Could not retrieve swap chain description");
		RB_ASSERT_FATAL_RELEASE_D3D(m_NativeSwapChain->ResizeBuffers(m_BackBufferCount, width, height,
			swap_chain_desc.BufferDesc.Format, swap_chain_desc.Flags), "Could not resize swap chain buffers");

		m_CurrentBackBufferIndex = m_NativeSwapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews();
	}
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE SwapChain::GetCurrentDescriptorHandleCPU() const
	{
		return GetDescriptorHandleCPU(m_CurrentBackBufferIndex);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE SwapChain::GetDescriptorHandleCPU(uint32_t back_buffer_index) const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			back_buffer_index, m_DescriptorIncrementSize);
	}

	void SwapChain::CreateDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = m_BackBufferCount;
		desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get2()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)), 
			"Could not create descriptor heap for swap chain buffers");
	}
	
	void SwapChain::UpdateRenderTargetViews()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (uint32_t back_buffer_index = 0; back_buffer_index < m_BackBufferCount; ++back_buffer_index)
		{
			GPtr<ID3D12Resource> back_buffer;
			RB_ASSERT_FATAL_RELEASE_D3D(m_NativeSwapChain->GetBuffer(back_buffer_index, IID_PPV_ARGS(&back_buffer)), "Could not retrieve back buffer from swap chain");

			g_GraphicsDevice->Get2()->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_handle);

			m_BackBuffers[back_buffer_index] = back_buffer;
			rtv_handle.Offset(m_DescriptorIncrementSize);
		}
	}
}
