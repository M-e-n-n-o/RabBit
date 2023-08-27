#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 extension library.
#include <D3DX12/d3dx12.h>

namespace RB::Graphics::Native
{
	class DeviceEngine;

	class GraphicsDevice
	{
	public:
		GraphicsDevice();
		~GraphicsDevice();

				bool IsFormatSupported(DXGI_FORMAT format);
		static	bool IsFeatureSupported(DXGI_FEATURE feature);

		GPtr<ID3D12Device2> Get() const { return m_NativeDevice; }
		GPtr<IDXGIAdapter4> GetAdapter() const { return m_NativeAdapter; }

		// Multi-engine: https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
		DeviceEngine* GetCopyEngine();
		DeviceEngine* GetComputeEngine();
		DeviceEngine* GetGraphicsEngine();

	private:
		void CreateAdapter();
		void CreateDevice();

		GPtr<ID3D12Device2>	m_NativeDevice;
		GPtr<IDXGIAdapter4>	m_NativeAdapter;

		DeviceEngine*		m_CopyEngine;
		DeviceEngine*		m_ComputeEngine;
		DeviceEngine*		m_GraphicsEngine;
	};

	extern GraphicsDevice* g_GraphicsDevice;
}