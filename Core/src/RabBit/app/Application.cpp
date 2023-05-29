#include "RabBitPch.h"
#include "Application.h"

#include "graphics/window/NativeWindow.h"
#include "graphics/window/SwapChain.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/CommandQueue.h"
#include "graphics/CommandList.h"
#include "input/events/ApplicationEvent.h"

using namespace RB::Graphics;
using namespace RB::Graphics::Window;
using namespace RB::Input::Events;

namespace RB
{
	Application::Application()
		:	m_Initialized(false),
			m_ShouldStop(false)
				
	{
		RB_LOG_RELEASE("Welcome to the RabBit Engine");
		RB_LOG_RELEASE("Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);	
	}

	Application::~Application()
	{

	}

	void Application::Start(void* window_instance)
	{
		const uint32_t width  = 1920;
		const uint32_t height = 1080;

		g_NativeWindow = new NativeWindow(this);
		g_NativeWindow->RegisterWindowCLass((HINSTANCE) window_instance, L"DX12WindowClass");
		g_NativeWindow->CreateWindow((HINSTANCE) window_instance, L"DX12WindowClass", L"RabBit App", width, height);

		g_GraphicsDevice = new GraphicsDevice();

		CommandQueue command_queue = CommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE);

		g_SwapChain = new SwapChain(command_queue.Get(), width, height);

		CommandList command_list = CommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, g_SwapChain->GetBackBufferCount(), g_SwapChain->GetCurrentBackBufferIndex());

		g_NativeWindow->ShowWindow();

		m_Initialized = true;

		// Initialize app user
		Start();
	}

	void Application::Run()
	{
		while (!m_ShouldStop)
		{
			g_NativeWindow->ProcessEvents();
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

	void Application::OnEvent(Event& event)
	{
		if (!m_Initialized)
		{
			return;
		}

		BindEvent<WindowCloseEvent>([this](WindowCloseEvent& window_event)
		{
			RB_LOG("Window close event received");
			m_ShouldStop = true;
		}, event);

		//BindEvent<WindowCreatedEvent>(RB_BIND_EVENT_FN(test), event);
	}
}