#include "RabBitCommon.h"
#include "SwapChain.h"
#include "graphics/native/GraphicsDevice.h"
#include "graphics/native/window/NativeWindow.h"

namespace RB::Graphics::Native::Window
{
	SwapChain::SwapChain(GPtr<IDXGIFactory2> factory, GPtr<ID3D12CommandQueue> command_queue, HWND window_handle, uint32_t width, uint32_t height, uint32_t buffer_count, DXGI_FORMAT format, bool transparency_support)
		: m_BackBufferCount(buffer_count)
		, m_Width(width)
		, m_Height(height)
		, m_BackBufferFormat(format)
		, m_UseComposition(transparency_support)
	{
		RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, g_GraphicsDevice->IsFormatSupported(format), "Device does not support passed in format, can not create SwapChain");

		m_IsTearingSupported = g_GraphicsDevice->IsFeatureSupported(DXGI_FEATURE_PRESENT_ALLOW_TEARING); // And if exlusive fullscreen is not active

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
		swap_chain_desc.AlphaMode	= m_UseComposition ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_UNSPECIFIED;
		swap_chain_desc.Flags		= m_IsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		GPtr<IDXGISwapChain1> swap_chain1;

		if (m_UseComposition)
		{
			RB_ASSERT_FATAL_RELEASE_D3D(factory->CreateSwapChainForComposition(
				command_queue.Get(),
				&swap_chain_desc,
				nullptr,
				&swap_chain1
			), "Could not create composition swap chain");
		}
		else
		{
			RB_ASSERT_FATAL_RELEASE_D3D(factory->CreateSwapChainForHwnd(
				command_queue.Get(),
				window_handle,
				&swap_chain_desc,
				nullptr,
				nullptr,
				&swap_chain1
			), "Could not create swap chain for HWND");
		}

		RB_ASSERT_FATAL_RELEASE_D3D(factory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER), "Could not hint window to not automatically handle Alt-Enter");

		RB_ASSERT_FATAL_RELEASE_D3D(swap_chain1.As(&m_NativeSwapChain), "Could not convert the swap chain to abstraction level 4");

		m_CurrentBackBufferIndex = m_NativeSwapChain->GetCurrentBackBufferIndex();

		CreateDescriptorHeap();
		m_DescriptorIncrementSize = g_GraphicsDevice->Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		m_BackBuffers = new GPtr<ID3D12Resource>[m_BackBufferCount];
		UpdateRenderTargetViews();

		if (m_UseComposition)
		{
			CreateCompositionObjects(window_handle);
		}
	}
	
	SwapChain::~SwapChain()
	{
		delete[] m_BackBuffers;
	}

	void SwapChain::Present(const VsyncMode& vsync_mode)
	{
		bool vsync_enabled = vsync_mode != VsyncMode::Off;
		UINT sync_interval = (UINT) vsync_mode;
		UINT present_flags = (m_IsTearingSupported && !vsync_enabled) ? DXGI_PRESENT_ALLOW_TEARING : 0; // DXGI_PRESENT_ALLOW_TEARING cannot be here in exclusive fullscreen!

		RB_ASSERT_FATAL_RELEASE_D3D(m_NativeSwapChain->Present(sync_interval, present_flags), "Failed to present frame");

		m_CurrentBackBufferIndex = m_NativeSwapChain->GetCurrentBackBufferIndex();

		if (m_UseComposition)
		{
			RB_ASSERT_FATAL_RELEASE_D3D(m_CompositionDevice->Commit(), "Could not commit the composition device");
		}
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

		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)), 
			"Could not create descriptor heap for swap chain buffers");
	}
	
	void SwapChain::UpdateRenderTargetViews()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (uint32_t back_buffer_index = 0; back_buffer_index < m_BackBufferCount; ++back_buffer_index)
		{
			GPtr<ID3D12Resource> back_buffer;
			RB_ASSERT_FATAL_RELEASE_D3D(m_NativeSwapChain->GetBuffer(back_buffer_index, IID_PPV_ARGS(&back_buffer)), "Could not retrieve back buffer from swap chain");

			g_GraphicsDevice->Get()->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_handle);

			m_BackBuffers[back_buffer_index] = back_buffer;
			rtv_handle.Offset(m_DescriptorIncrementSize);

			std::wstring name = L"Swapchain Buffer " + std::to_wstring(back_buffer_index);
			m_BackBuffers[back_buffer_index]->SetName(name.c_str());
		}
	}
	
	void SwapChain::CreateCompositionObjects(HWND window_handle)
	{
		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get11On12().As(&m_DeviceForComposition), "Could not query the DXGI device from the D3D11On12 device");

		RB_ASSERT_FATAL_RELEASE_D3D(DCompositionCreateDevice(m_DeviceForComposition.Get(), IID_PPV_ARGS(&m_CompositionDevice)), "Could not create composition device");

		m_CompositionDevice->CreateTargetForHwnd(window_handle, true, m_CompositionTarget.GetAddressOf());

		GPtr<IDCompositionVisual> visual;
		RB_ASSERT_FATAL_RELEASE_D3D(m_CompositionDevice->CreateVisual(visual.GetAddressOf()), "Could not create composition visual");

		RB_ASSERT_FATAL_RELEASE_D3D(visual->SetContent(m_NativeSwapChain.Get()), "Could not set the swapchain as the composition content");

		RB_ASSERT_FATAL_RELEASE_D3D(m_CompositionTarget->SetRoot(visual.Get()), "Could not set the visual as the composition target");
	}
}
