#include "RabBitPch.h"
#include "Application.h"

#include "graphics/Window.h"
#include "graphics/native/window/SwapChain.h"
#include "graphics/native/GraphicsDevice.h"
#include "graphics/native/CommandQueue.h"
#include "graphics/native/CommandList.h"
#include "input/events/ApplicationEvent.h"

using namespace RB::Graphics;
using namespace RB::Graphics::Native;
using namespace RB::Graphics::Native::Window;
using namespace RB::Input::Events;

namespace RB
{
	CommandQueue* _CommandQueue = nullptr;
	CommandList* _CommandList = nullptr;
	uint32_t _FenceValues[3] = {};

	Application::Application(AppInfo& info)
		: m_StartAppInfo(info)
		, m_Initialized(false)
		, m_ShouldStop(false)
				
	{
		RB_LOG_RELEASE(LOGTAG_GRAPHICS, "Welcome to the RabBit Engine");
		RB_LOG_RELEASE(LOGTAG_GRAPHICS, "Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);
	}

	Application::~Application()
	{

	}

	void Application::Start(void* window_instance)
	{
		g_GraphicsDevice = new GraphicsDevice();

		_CommandQueue = new CommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE);

		m_Window = new Graphics::Window(window_instance, _CommandQueue, this, m_StartAppInfo.name, m_StartAppInfo.windowWidth, m_StartAppInfo.windowHeight);

		_CommandList = new CommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, g_SwapChain->GetBackBufferCount(), g_SwapChain->GetCurrentBackBufferIndex());

		m_Initialized = true;

		// Initialize app user
		Start();
	}

	void Application::Run()
	{
		while (!m_ShouldStop)
		{
			//g_NativeWindow->ProcessEvents();
			m_Window->Update();

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
		delete _CommandQueue;
		delete m_Window;
		delete g_GraphicsDevice;		
	}

	void Application::Render()
	{
		if (m_Window->IsMinimized())
		{
			return;
		}

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


			_FenceValues[g_SwapChain->GetCurrentBackBufferIndex()] = _CommandQueue->SignalFence	();

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
			RB_LOG(LOGTAG_EVENT, "Window resize event received");

			uint32_t width = resize_event.GetWidth();
			uint32_t height = resize_event.GetHeight();

			_CommandQueue->Flush();

			m_Window->Resize(width, height);
		}, event);

		BindEvent<WindowCloseEvent>([this](WindowCloseEvent& close_event)
		{
			RB_LOG(LOGTAG_EVENT, "Window close event received");
			m_ShouldStop = true;
		}, event);
	}
}