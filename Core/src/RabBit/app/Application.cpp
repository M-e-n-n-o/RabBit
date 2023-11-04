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

#include <d3dcompiler.h>

using namespace RB::Graphics;
using namespace RB::Graphics::Native;
using namespace RB::Graphics::Native::Window;
using namespace RB::Input::Events;
using namespace RB::Input;

namespace RB
{
	DeviceEngine* _GraphicsEngine = nullptr;
	uint32_t _FenceValues[Graphics::Window::BACK_BUFFER_COUNT] = {};

	Graphics::Window* _SecondWindow;

	GPtr<ID3D12PipelineState> _Pso;

	float _Value = 0;

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

	void Application::Start(const char* launch_args)
	{
		RB_LOG(LOGTAG_MAIN, "");
		RB_LOG(LOGTAG_MAIN, "============== STARTUP ==============");
		RB_LOG(LOGTAG_MAIN, "");

		g_GraphicsDevice = new GraphicsDevice();

		_GraphicsEngine = g_GraphicsDevice->GetGraphicsEngine();

		//_SecondWindow = new Graphics::Window("Test", 1280, 720, kWindowStyle_Borderless);
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
		UINT flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#ifdef RB_CONFIG_DEBUG
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; // | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif

		GPtr<ID3DBlob> vs, ps, error;
		HRESULT result = D3DCompileFromFile(L"test.hlsl", NULL, NULL, "vs_main", "vs_4_0", flags, 0, &vs, &error);
		if (result != S_OK)
		{
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Could not compile vertex shader, error: %s", (char*)error->GetBufferPointer());
		}
		result = D3DCompileFromFile(L"test.hlsl", NULL, NULL, "ps_main", "ps_4_0", flags, 0, &ps, &error);
		if (result != S_OK)
		{
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Could not compile vertex shader, error: %s", (char*)error->GetBufferPointer());
		}

		D3D12_ROOT_SIGNATURE_DESC signature_desc = {};
		signature_desc.NumParameters		= 0;
		signature_desc.pParameters			= nullptr;
		signature_desc.NumStaticSamplers	= 0;
		signature_desc.pStaticSamplers		= nullptr;
		signature_desc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		GPtr<ID3DBlob> root_signature_blob;
		GPtr<ID3DBlob> error_blob;
		RB_ASSERT_FATAL_RELEASE_D3D(D3D12SerializeRootSignature(&signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &error_blob), "Could not serialize root signature");

		GPtr<ID3D12RootSignature> root_signature;
		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)), "Could not create root signature");

		D3D12_BLEND_DESC blend_desc = {};
		blend_desc.AlphaToCoverageEnable					= false;
		blend_desc.IndependentBlendEnable					= false;
		blend_desc.RenderTarget[0].BlendEnable				= false;
		blend_desc.RenderTarget[0].RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_INPUT_ELEMENT_DESC input_elements[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0  },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0  },
		};		

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
		pso_desc.pRootSignature			= root_signature.Get();
		pso_desc.VS						= { vs->GetBufferPointer(), vs->GetBufferSize() };
		pso_desc.PS						= { ps->GetBufferPointer(), ps->GetBufferSize() };
		//pso_desc.StreamOutput			= ;
		pso_desc.BlendState				= blend_desc;
		pso_desc.SampleMask				= 0;
		pso_desc.RasterizerState		= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pso_desc.DepthStencilState		= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		pso_desc.InputLayout			= { input_elements, 2 };
		//pso_desc.IBStripCutValue		= D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		pso_desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pso_desc.NumRenderTargets		= 1;
		pso_desc.RTVFormats[0]			= m_Window->GetSwapChain()->GetBackBufferFormat();
		pso_desc.DSVFormat				= DXGI_FORMAT_UNKNOWN;
		pso_desc.SampleDesc				= { 1, 0 };
		pso_desc.NodeMask				= 0;
		//pso_desc.CachedPSO			= NULL;
		pso_desc.Flags					= D3D12_PIPELINE_STATE_FLAG_NONE;

		_Pso = g_PipelineManager->GetGraphicsPipeline(pso_desc);

		while (!m_ShouldStop)
		{
			_Value += 0.01f;
			_Value = fmodf(_Value, 1);

			// Update application
			OnUpdate();

			// Render
			StartRenderFrame();
			Render();
			FinishRenderFrame();

			// Poll inputs and update window
			m_Window->Update();

			//if (_SecondWindow->IsValid())
			//	_SecondWindow->Update();

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

		//if (_SecondWindow->IsValid())
		//	delete _SecondWindow;

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
				RB_PROFILE_GPU_SCOPED(d3d_list, "Clear");

				CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					back_buffer.Get(),
					D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
				);

				d3d_list->ResourceBarrier(1, &barrier);

				FLOAT clear_color[] = { _Value, 0.1f, 0.1f, 0.0f };

				d3d_list->ClearRenderTargetView(m_Window->GetSwapChain()->GetCurrentDescriptorHandleCPU(), clear_color, 0, nullptr);
			}

			// Draw quad
			//{
			//	RB_PROFILE_GPU_SCOPED(d3d_list, "Draw");


			//}

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
				if (_SecondWindow->IsValid())
					_SecondWindow->Resize(200, 200, 200, 100);
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