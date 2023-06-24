#pragma once

#include "utils/Ptr.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 extension library.
#include <D3DX12/d3dx12.h>

namespace RB::Graphics::Native
{
	class GpuEngine;

	class GraphicsDevice
	{
	public:
		GraphicsDevice();
		~GraphicsDevice();

				bool IsFormatSupported(DXGI_FORMAT format);
		static	bool IsFeatureSupported(DXGI_FEATURE feature);

		GPtr<ID3D12Device2> Get2() const { return m_NativeDevice; }
		GPtr<IDXGIAdapter4> GetAdapter4() const { return m_NativeAdapter; }

		// Multi-engine: https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
		GpuEngine* GetCopyEngine();
		GpuEngine* GetComputeEngine();
		GpuEngine* GetGraphicsEngine();

	private:
		void CreateAdapter();
		void CreateDevice();

		GPtr<ID3D12Device2> m_NativeDevice;
		GPtr<IDXGIAdapter4> m_NativeAdapter;

		GpuEngine*			m_CopyEngine;
		GpuEngine*			m_ComputeEngine;
		GpuEngine*			m_GraphicsEngine;
	};

	extern GraphicsDevice* g_GraphicsDevice;
}