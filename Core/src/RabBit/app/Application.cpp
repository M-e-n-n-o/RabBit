#include "RabBitCommon.h"
#include "Application.h"

#include "graphics/Window.h"
#include "graphics/native/window/SwapChain.h"
#include "graphics/native/GraphicsDevice.h"
#include "graphics/native/DeviceEngine.h"
#include "graphics/native/resource/ResourceManager.h"
#include "graphics/native/pipeline/Pipeline.h"

#include "input/events/WindowEvent.h"
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

	float value = 0;

	Application::Application(AppInfo& info)
		: EventListener(kEventCat_All)
		, m_StartAppInfo(info)
		, m_Initialized(false)
		, m_ShouldStop(false)
		, m_FrameIndex(0)
				
	{
		RB_LOG_RELEASE(LOGTAG_MAIN, "Welcome to the RabBit Engine");
		RB_LOG_RELEASE(LOGTAG_MAIN, "Version: %s.%s.%s", RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);
	}

	Application::~Application()
	{

	}

	void Application::Start()
	{
		RB_LOG(LOGTAG_MAIN, "");
		RB_LOG(LOGTAG_MAIN, "============== STARTUP ==============");
		RB_LOG(LOGTAG_MAIN, "");

		g_GraphicsDevice = new GraphicsDevice();

		_GraphicsEngine = g_GraphicsDevice->GetGraphicsEngine();

		SecondWindow = new Graphics::Window("Test", 1280, 720, kWindowStyle_Borderless);
		m_Window = new Graphics::Window(m_StartAppInfo.name, m_StartAppInfo.windowWidth, m_StartAppInfo.windowHeight, kWindowStyle_SemiTransparent);

		g_PipelineManager = new PipelineManager();

		m_Initialized = true;

		RB_LOG(LOGTAG_MAIN, "");
		RB_LOG(LOGTAG_MAIN, "========== STARTUP COMPLETE =========");
		RB_LOG(LOGTAG_MAIN, "");

		// Initialize app user
		RB_LOG(LOGTAG_MAIN, "Starting user's application: %s", m_StartAppInfo.name);
		OnStart();

		RB_LOG(LOGTAG_MAIN, "");
		RB_LOG(LOGTAG_MAIN, "======== STARTING MAIN LOOP =========");
		RB_LOG(LOGTAG_MAIN, "");
	}

	void Application::Run()
	{
		D3D12_ROOT_SIGNATURE_DESC signature_desc = {};
		signature_desc.NumParameters		= 0;
		signature_desc.pParameters			= nullptr;
		signature_desc.NumStaticSamplers	= 0;
		signature_desc.pStaticSamplers		= nullptr;
		signature_desc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_NONE;

		GPtr<ID3DBlob> root_signature_blob;
		GPtr<ID3DBlob> error_blob;
		RB_ASSERT_FATAL_RELEASE_D3D(D3D12SerializeRootSignature(&signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_1, &root_signature_blob, &error_blob), "Could not serialize root signature");

		GPtr<ID3D12RootSignature> root_signature;
		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)), "Could not create root signature");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
		pso_desc.pRootSignature		= root_signature.Get();
		pso_desc.VS					= 

		g_PipelineManager->GetGraphicsPipeline(pso_desc);



		while (!m_ShouldStop)
		{
			value += 0.01f;
			value = fmodf(value, 1);

			// Update application
			OnUpdate();

			// Render
			StartRenderFrame();
			Render();
			FinishRenderFrame();

			// Poll inputs and update window
			m_Window->Update();

			if (SecondWindow->IsValid())
				SecondWindow->Update();

			m_FrameIndex++;
		}
	}

	void Application::Shutdown()
	{
		RB_LOG(LOGTAG_MAIN, "");
		RB_LOG(LOGTAG_MAIN, "============= SHUTDOWN ==============");
		RB_LOG(LOGTAG_MAIN, "");

		// Shutdown app user
		OnStop();

		_GraphicsEngine->WaitForIdle();

		if (SecondWindow->IsValid())
			delete SecondWindow;

		delete g_PipelineManager;
		delete m_Window;
		delete g_GraphicsDevice;	

		RB_LOG(LOGTAG_MAIN, "");
		RB_LOG(LOGTAG_MAIN, "========= SHUTDOWN COMPLETE =========");
		RB_LOG(LOGTAG_MAIN, "");
	}

	void Application::StartRenderFrame()
	{

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

				FLOAT clear_color[] = { value, 0.1f, 0.1f, 0.0f };

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

	void Application::FinishRenderFrame()
	{

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

		//BindEvent<WindowRenderEvent>([this](WindowRenderEvent& render_event)
		//{
		//	Render();
		//}, event);

		BindEvent<WindowCloseRequestEvent>([this](WindowCloseRequestEvent& close_event)
		{
			RB_LOG(LOGTAG_EVENT, "Window close event received");
			m_ShouldStop = true;
		}, event);
	}
}