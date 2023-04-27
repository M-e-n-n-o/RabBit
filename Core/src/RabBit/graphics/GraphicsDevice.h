#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

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

		Microsoft::WRL::ComPtr<ID3D12Device2> m_NativeDevice;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> m_NativeAdapter;
	};
}