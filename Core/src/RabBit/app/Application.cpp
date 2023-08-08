#include "RabBitCommon.h"
#include "Application.h"

#include "graphics/Window.h"
#include "graphics/native/window/SwapChain.h"
#include "graphics/native/GraphicsDevice.h"
#include "graphics/native/GraphicsDeviceEngine.h"
#include "input/events/ApplicationEvent.h"

using namespace RB::Graphics;
using namespace RB::Graphics::Native;
using namespace RB::Graphics::Native::Window;
using namespace RB::Input::Events;

namespace RB
{
	GraphicsDeviceEngine* _GraphicsEngine = nullptr;
	uint32_t _FenceValues[Graphics::Window::BACK_BUFFER_COUNT] = {};

	Application::Application(AppInfo& info)
		: m_StartAppInfo(info)
		, m_Initialized(false)
		, m_ShouldStop(false)
				
	{
		RB_LOG_RELEASE(LOGTAG_MAIN, "Welcome to the RabBit Engine");
		RB_LOG_RELEASE(LOGTAG_MAIN, "Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);
	}

	Application::~Application()
	{

	}

	void Application::Start(void* window_instance)
	{
		g_GraphicsDevice = new GraphicsDevice();

		_GraphicsEngine = g_GraphicsDevice->GetGraphicsEngine();

		m_Window = new Graphics::Window(window_instance, this, m_StartAppInfo.name, m_StartAppInfo.windowWidth, m_StartAppInfo.windowHeight);

		m_Initialized = true;

		// Initialize app user
		RB_LOG(LOGTAG_MAIN, "Starting application: %ls", m_StartAppInfo.name);
		Start();
	}

	void Application::Run()
	{
		while (!m_ShouldStop)
		{
			m_Window->Update();

			// Only do updates, rendering is called via events
			Update();
		}
	}

	void Application::Shutdown()
	{
		// Shutdown app user
		Stop();

		_GraphicsEngine->WaitForIdle();

		delete m_Window;
		delete g_GraphicsDevice;		
	}

	void Application::Render()
	{
		if (m_Window->IsMinimized())
		{
			return;
		}

		GPtr<ID3D12GraphicsCommandList2> command_list = _GraphicsEngine->GetCommandList();

		auto back_buffer = g_SwapChain->GetCurrentBackBuffer();

		// Clear the render target
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				back_buffer.Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
			);

			command_list->ResourceBarrier(1, &barrier);

			FLOAT clear_color[] = { 0.3f, 1.0f, 0.7f, 1.0f };

			command_list->ClearRenderTargetView(g_SwapChain->GetCurrentDescriptorHandleCPU(), clear_color, 0, nullptr);
		}

		// Present
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				back_buffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
			);

			command_list->ResourceBarrier(1, &barrier);

			_FenceValues[g_SwapChain->GetCurrentBackBufferIndex()] = _GraphicsEngine->ExecuteCommandList(command_list);

			g_SwapChain->Present(true);

			_GraphicsEngine->WaitForFenceValue(_FenceValues[g_SwapChain->GetCurrentBackBufferIndex()]);
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

			_GraphicsEngine->WaitForIdle();

			m_Window->Resize(width, height);
		}, event);

		BindEvent<WindowCloseEvent>([this](WindowCloseEvent& close_event)
		{
			RB_LOG(LOGTAG_EVENT, "Window close event received");
			m_ShouldStop = true;
		}, event);
	}
}