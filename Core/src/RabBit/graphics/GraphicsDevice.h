#pragma once

#include "utils/Gptr.h"

// DirectX 12 specific headers.
#include <dxgi1_6.h>

// D3D12 extension library.
#include <D3DX12/d3dx12.h>

namespace RB::Graphics
{
	class GraphicsDevice
	{
	public:
		GraphicsDevice();
		~GraphicsDevice();

	private:
		void CreateAdapter();
		void CreateDevice();

		GPtr<ID3D12Device2> m_NativeDevice;
		GPtr<IDXGIAdapter4> m_NativeAdapter;
	};

	extern GraphicsDevice* g_GraphicsDevice;
}