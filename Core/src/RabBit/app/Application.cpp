#include "RabBitPch.h"
#include "Application.h"

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

namespace RB
{
	Application::Application()
	{
		RB_LOG("RabBit Engine version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);

		RB_ASSERT(1 == 1);

		Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface;
		RB_ASSERT_D3D(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
		debug_interface->EnableDebugLayer();
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		while (true)
		{
			Update();
		}
	}
}