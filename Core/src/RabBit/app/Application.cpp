#include "RabBitCommon.h"
#include "Application.h"

#include "graphics/Window.h"
#include "graphics/native/window/SwapChain.h"
#include "graphics/native/GraphicsDevice.h"
#include "graphics/native/DeviceEngine.h"
#include "input/events/ApplicationEvent.h"

#include "input/events/KeyEvent.h"
#include "input/KeyCodes.h"

using namespace RB::Graphics;
using namespace RB::Graphics::Native;
using namespace RB::Graphics::Native::Window;
using namespace RB::Input::Events;
using namespace RB::Input;

namespace RB
{
	DeviceEngine* _GraphicsEngine = nullptr;
	uint32_t _FenceValues[Graphics::Window::BACK_BUFFER_COUNT] = {};

	Graphics::Window* SecondWindow;

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

	void Application::Start()
	{
		RB_LOG(LOGTAG_GRAPHICS, "");
		RB_LOG(LOGTAG_GRAPHICS, "============== STARTUP ==============");
		RB_LOG(LOGTAG_GRAPHICS, "");

		g_GraphicsDevice = new GraphicsDevice();

		_GraphicsEngine = g_GraphicsDevice->GetGraphicsEngine();


		SecondWindow = new Graphics::Window("Test", nullptr, 1280, 720, kWindowStyle_Borderless);
		m_Window = new Graphics::Window(m_StartAppInfo.name, this, m_StartAppInfo.windowWidth, m_StartAppInfo.windowHeight, kWindowStyle_SemiTransparent);

		m_Initialized = true;

		RB_LOG(LOGTAG_GRAPHICS, "");
		RB_LOG(LOGTAG_GRAPHICS, "========== STARTUP COMPLETE =========");
		RB_LOG(LOGTAG_GRAPHICS, "");

		// Initialize app user
		RB_LOG(LOGTAG_MAIN, "Starting user's application: %s", m_StartAppInfo.name);
		OnStart();

		RB_LOG(LOGTAG_GRAPHICS, "");
		RB_LOG(LOGTAG_GRAPHICS, "======== STARTING MAIN LOOP =========");
		RB_LOG(LOGTAG_GRAPHICS, "");
	}

	void Application::Run()
	{
		while (!m_ShouldStop)
		{
			m_Window->Update();

			if (SecondWindow->IsValid())
				SecondWindow->Update();

			// Only do updates, rendering is called via events
			OnUpdate();
		}
	}

	void Application::Shutdown()
	{
		RB_LOG(LOGTAG_GRAPHICS, "");
		RB_LOG(LOGTAG_GRAPHICS, "============= SHUTDOWN ==============");
		RB_LOG(LOGTAG_GRAPHICS, "");

		// Shutdown app user
		OnStop();

		_GraphicsEngine->WaitForIdle();

		if (SecondWindow->IsValid())
			delete SecondWindow;

		delete m_Window;
		delete g_GraphicsDevice;	

		RB_LOG(LOGTAG_GRAPHICS, "");
		RB_LOG(LOGTAG_GRAPHICS, "========= SHUTDOWN COMPLETE =========");
		RB_LOG(LOGTAG_GRAPHICS, "");
	}

	void Application::Render()
	{
		if (m_Window->IsMinimized())
		{
			return;
		}

		CommandList* command_list = _GraphicsEngine->GetCommandList();
		ID3D12GraphicsCommandList2* d3d_list = command_list->GetCommandList();

		auto back_buffer = m_Window->GetSwapChain()->GetCurrentBackBuffer();

		{
			// Clear the render target
			{
				RB_PROFILE_GPU_SCOPED(d3d_list, "Frame");

				CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					back_buffer.Get(),
					D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
				);

				d3d_list->ResourceBarrier(1, &barrier);

				FLOAT clear_color[] = { 0.3f, 0.1f, 0.1f, 0.0f };

				d3d_list->ClearRenderTargetView(m_Window->GetSwapChain()->GetCurrentDescriptorHandleCPU(), clear_color, 0, nullptr);
			}

			// Present
			{
				CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					back_buffer.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
				);

				d3d_list->ResourceBarrier(1, &barrier);

				uint64_t value = m_Window->GetSwapChain()->GetCurrentBackBufferIndex();
				_FenceValues[m_Window->GetSwapChain()->GetCurrentBackBufferIndex()] = _GraphicsEngine->ExecuteCommandList(command_list);

				m_Window->GetSwapChain()->Present(VsyncMode::On);

				value = m_Window->GetSwapChain()->GetCurrentBackBufferIndex();
				_GraphicsEngine->WaitForFenceValue(_FenceValues[m_Window->GetSwapChain()->GetCurrentBackBufferIndex()]);
			}

		}
	}

	void Application::OnEvent(Event& event)
	{
		if (!m_Initialized)
		{
			return;
		}

		BindEvent<KeyPressedEvent>([this](KeyPressedEvent& e)
		{
			if (e.GetKeyCode() == KeyCode::K)
			{
				if (SecondWindow->IsValid())
					SecondWindow->Resize(200, 200, 200, 100);
			}
		}, event);

		// BindEvent<EventType>(RB_BIND_EVENT_FN(Class::Method), event);

		BindEvent<WindowRenderEvent>([this](WindowRenderEvent& render_event)
		{
			Render();
		}, event);

		BindEvent<WindowCloseEvent>([this](WindowCloseEvent& close_event)
		{
			RB_LOG(LOGTAG_EVENT, "Window close event received");
			m_ShouldStop = true;
		}, event);
	}
}