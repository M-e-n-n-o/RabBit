#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 extension library.
#include <D3DX12/d3dx12.h>

#include <d3d11.h>
#include <d3d11on12.h>

namespace RB::Graphics::Native
{
	class DeviceEngine;
	
	struct MonitorInfo
	{
		enum Rotation
		{
			kRotation_None = 0,
			kRotation_90,
			kRotation_180,
			kRotation_270
		};

		char*					name;
		RB::Math::Float2		resolution;
		Rotation				rotation;
		uint32_t				bitsPerColor;
		DXGI_COLOR_SPACE_TYPE	colorSpace;
		float					minLuminance;
		float					maxLuminance;
		float					maxFullscreenLuminance;
	};

	struct GraphicsCardInfo
	{
		const char*	name;
		int64_t		videoMemory;
		int64_t		sharedSystemMemory;
	};

	class GraphicsDevice
	{
	public:
		GraphicsDevice();
		~GraphicsDevice();

			   bool IsFormatSupported(DXGI_FORMAT format);
		static bool IsFeatureSupported(DXGI_FEATURE feature);

		GPtr<ID3D12Device2> Get() const { return m_NativeDevice; }
		GPtr<IDXGIAdapter4> GetAdapter() const { return m_NativeAdapter; }

		// Not recommended to use for regular rendering (currently only used for Direct Composition)
		GPtr<ID3D11On12Device> Get11On12();

		// Multi-engine: https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
		DeviceEngine* GetCopyEngine();
		DeviceEngine* GetComputeEngine();
		DeviceEngine* GetGraphicsEngine();

		const GraphicsCardInfo GetGraphicsCardInfo() const { return m_GpuInfo; }
		const List<MonitorInfo> GetMonitors() const { return m_Monitors; }

	private:
		void CreateAdapter();
		void CreateMonitors();
		void CreateDevice();

		GPtr<ID3D12Device2>			m_NativeDevice;
		GPtr<IDXGIAdapter4>			m_NativeAdapter;

		// Only created when required
		GPtr<ID3D11On12Device>		m_11On12Device;
		GPtr<ID3D11DeviceContext>	m_11DeviceContext;

		DeviceEngine*				m_CopyEngine;
		DeviceEngine*				m_ComputeEngine;
		DeviceEngine*				m_GraphicsEngine;

		GraphicsCardInfo			m_GpuInfo;
		List<MonitorInfo>			m_Monitors;
	};

	extern GraphicsDevice* g_GraphicsDevice;
}