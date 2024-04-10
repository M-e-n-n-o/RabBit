#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 extension library.
#include <d3dx12/d3dx12.h>

// Direct Composition, for transparency support
#include <dcomp.h>

namespace RB::Graphics::D3D12
{
	enum class VsyncMode : uint8_t
	{
		Off		= 0,
		On		= 1,
		Half	= 2,
		Quarter	= 3,
		Eighth	= 4
	};

	class SwapChain
	{
	public:
		SwapChain(GPtr<IDXGIFactory2> factory, GPtr<ID3D12CommandQueue> command_queue, HWND window_handle, const uint32_t width, const uint32_t height, const uint32_t buffer_count = 2u,
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool transparency_support = false);
		~SwapChain();

		void Present(const VsyncMode& vsync_mode);

		void Resize(const uint32_t width, const uint32_t height);

		GPtr<IDXGISwapChain4> Get4() const { return m_NativeSwapChain; }
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		uint32_t GetBackBufferCount() const { return m_BackBufferCount; }
		uint32_t GetCurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }
		DXGI_FORMAT GetBackBufferFormat() const { return m_BackBufferFormat; }
		GPtr<ID3D12Resource> GetCurrentBackBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex]; }
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentDescriptorHandleCPU() const;
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandleCPU(uint32_t back_buffer_index) const;

	private:
		void CreateDescriptorHeap();
		void UpdateRenderTargetViews();
		void CreateCompositionObjects(HWND window_handle);

		GPtr<IDXGISwapChain4>		m_NativeSwapChain;
		GPtr<ID3D12DescriptorHeap>	m_DescriptorHeap;
		GPtr<ID3D12Resource>*		m_BackBuffers;

		bool						m_UseComposition;
		GPtr<IDXGIDevice>			m_DeviceForComposition;
		GPtr<IDCompositionDevice>	m_CompositionDevice;
		GPtr<IDCompositionTarget>	m_CompositionTarget;

		uint32_t					m_DescriptorIncrementSize;
		uint32_t					m_CurrentBackBufferIndex;
		uint32_t					m_BackBufferCount;
		uint32_t					m_Width;
		uint32_t					m_Height;
		DXGI_FORMAT					m_BackBufferFormat;

		bool						m_IsTearingSupported;
	};
}