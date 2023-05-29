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
	CommandQueue* _CommandQueue = nullptr;
	CommandList* _CommandList = nullptr;
	uint32_t _FenceValues[3] = {};

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

		_CommandQueue = new CommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE);

		g_SwapChain = new SwapChain(_CommandQueue->Get(), width, height);

		_CommandList = new CommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, g_SwapChain->GetBackBufferCount(), g_SwapChain->GetCurrentBackBufferIndex());

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

			// Only do updates, rendering is called via events
			Update();
		}
	}

	void Application::Shutdown()
	{
		// Shutdown app user
		Stop();

		_CommandQueue->Flush();

		delete _CommandList;
		delete g_SwapChain;
		delete _CommandQueue;
		delete g_GraphicsDevice;
		delete g_NativeWindow;

		
	}

	void Application::Render()
	{
		RB_LOG("Start render frame");

		_CommandList->Reset(g_SwapChain->GetCurrentBackBufferIndex());
		auto back_buffer = g_SwapChain->GetCurrentBackBuffer();

		// Clear the render target
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				back_buffer.Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
			);

			_CommandList->Get()->ResourceBarrier(1, &barrier);

			FLOAT clear_color[] = { 0.3f, 1.0f, 0.7f, 1.0f };

			_CommandList->Get()->ClearRenderTargetView(g_SwapChain->GetCurrentDescriptorHandleCPU(), clear_color, 0, nullptr);
		}

		// Present
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				back_buffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
			);

			_CommandList->Get()->ResourceBarrier(1, &barrier);

			RB_ASSERT_FATAL_RELEASE_D3D(_CommandList->Get()->Close(), "Could not close command list");

			ID3D12CommandList* const command_lists[] = { _CommandList->Get().Get() };
			_CommandQueue->Get()->ExecuteCommandLists(_countof(command_lists), command_lists);


			_FenceValues[g_SwapChain->GetCurrentBackBufferIndex()] = _CommandQueue->PlaceFence();

			g_SwapChain->Present(true, true);

			_CommandQueue->WaitForFenceValue(_FenceValues[g_SwapChain->GetCurrentBackBufferIndex()]);
		}
	}

	void Application::OnEvent(Event& event)
	{
		if (!m_Initialized)
		{
			return;
		}

		// BindEvent<EventType>(RB_BIND_EVENT_FN(Class::Method), event);

		BindEvent<WindowRenderEvent>([this](WindowRenderEvent& render_event)
		{
			Render();
		}, event);

		BindEvent<WindowResizeEvent>([this](WindowResizeEvent& resize_event)
		{
			RB_LOG("Window resize event received");

			uint32_t width = resize_event.GetWidth();
			uint32_t height = resize_event.GetHeight();

			if (g_SwapChain->GetWidth() == width && g_SwapChain->GetHeight() == height)
			{
				return;
			}

			width = std::max(1u, width);
			height = std::max(1u, height);

			_CommandQueue->Flush();

			g_SwapChain->Resize(width, height);
		}, event);

		BindEvent<WindowCloseEvent>([this](WindowCloseEvent& close_event)
		{
			RB_LOG("Window close event received");
			m_ShouldStop = true;
		}, event);
	}
}