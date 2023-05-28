#include "RabBitPch.h"
#include "Application.h"

#include "graphics/window/NativeWindow.h"
#include "graphics/window/SwapChain.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/CommandQueue.h"
#include "graphics/CommandList.h"

using namespace RB::Graphics;
using namespace RB::Graphics::Window;

namespace RB
{
	Application::Application()
	{
		RB_LOG_RELEASE("Welcome to the RabBit Engine");
		RB_LOG_RELEASE("Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);	
	}

	Application::~Application()
	{

	}

	void Application::Start(HINSTANCE window_instance)
	{
		const uint32_t width  = 1920;
		const uint32_t height = 1080;

		g_NativeWindow = new NativeWindow();
		g_NativeWindow->RegisterWindowCLass(window_instance, L"DX12WindowClass");
		g_NativeWindow->CreateWindow(window_instance, L"DX12WindowClass", L"RabBit App", width, height);

		g_GraphicsDevice = new GraphicsDevice();

		CommandQueue command_queue = CommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE);

		g_SwapChain = new SwapChain(command_queue.Get(), width, height);

		CommandList command_list = CommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, g_SwapChain->GetBackBufferCount(), g_SwapChain->GetCurrentBackBufferIndex());

		g_NativeWindow->ShowWindow();

		// Initialize app user
		Start();
	}

	void Application::Run()
	{
		while (true)
		{
			Update();
		}
	}

	void Application::Shutdown()
	{
		// Shutdown app user
		Stop();

		delete g_SwapChain;
		delete g_GraphicsDevice;
		delete g_NativeWindow;
	}
}