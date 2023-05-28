#pragma once

#include "utils/Ptr.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 extension library.
#include <D3DX12/d3dx12.h>

namespace RB::Graphics::Window
{
	class SwapChain
	{
	public:
		SwapChain(GPtr<ID3D12CommandQueue> command_queue, const uint32_t width, const uint32_t height, const uint32_t buffer_count = 3u, 
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
		~SwapChain();

		GPtr<IDXGISwapChain4> Get4() const { return m_SwapChain; }
		uint32_t GetBackBufferCount() const { return m_BackBufferCount; }

	private:
		GPtr<IDXGISwapChain4>	m_SwapChain;

		uint32_t				m_CurrentBackBufferIndex;
		uint32_t				m_BackBufferCount;
		uint32_t				m_Width;
		uint32_t				m_Height;
	};

	extern SwapChain* g_SwapChain;
}