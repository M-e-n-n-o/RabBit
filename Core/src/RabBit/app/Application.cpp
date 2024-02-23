#include "RabBitCommon.h"
#include "Application.h"

#include "graphics/Window.h"
#include "graphics/d3d12/window/SwapChain.h"
#include "graphics/d3d12/GraphicsDevice.h"
#include "graphics/d3d12/DeviceQueue.h"
#include "graphics/d3d12/resource/ResourceManager.h"
#include "graphics/d3d12/resource/ResourceStateManager.h"
#include "graphics/d3d12/pipeline/Pipeline.h"
#include "graphics/d3d12/shaders/ShaderSystem.h"

#include "input/events/WindowEvent.h"
#include "input/events/KeyEvent.h"
#include "input/KeyCodes.h"

#include <d3dcompiler.h>
#include <fstream>
#include "graphics/d3d12/shaders/ShaderDefines.h"

using namespace RB::Graphics;
using namespace RB::Graphics::D3D12;
using namespace RB::Graphics::D3D12::Window;
using namespace RB::Input::Events;
using namespace RB::Input;

namespace RB
{
	DeviceQueue* _GraphicsQueue = nullptr;
	uint32_t _FenceValues[Graphics::Window::BACK_BUFFER_COUNT] = {};

	float _VertexData[] =
	{
		// Pos				Color
		-0.5f, -0.5f,		1, 0, 0,
		0, 0.5f,			0, 1, 0,
		0.5f, -0.5f,		0, 0, 1,
	};

	GPtr<ID3D12PipelineState> _Pso;
	GPtr<ID3D12RootSignature> _RootSignature;
	GPtr<ID3D12Resource> _VertexRes;
	GPtr<ID3D12Resource> _UploadResource;
	D3D12_VERTEX_BUFFER_VIEW _VaoView;

	float _Value = 0;

	Application::Application(AppInfo& info)
		: EventListener(kEventCat_All)
		, m_StartAppInfo(info)
		, m_Initialized(false)
		, m_ShouldStop(false)
		, m_FrameIndex(0)
		, m_CheckWindows(false)
				
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

		_GraphicsQueue = g_GraphicsDevice->GetGraphicsQueue();

		m_Windows.push_back(new Graphics::Window(m_StartAppInfo.name, m_StartAppInfo.windowWidth, m_StartAppInfo.windowHeight, kWindowStyle_None));
		m_Windows.push_back(new Graphics::Window("Test", 1280, 720, kWindowStyle_None));

		g_ResourceStateManager	= new ResourceStateManager();
		g_ResourceManager		= new ResourceManager(2);
		g_ShaderSystem			= new ShaderSystem();
		g_PipelineManager		= new PipelineManager();

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
		// Pipeline
		{
			UINT flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#ifdef RB_CONFIG_DEBUG
			flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; // | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif

			ShaderBlob* vs_blob = g_ShaderSystem->GetShaderBlob(VS_VertexColor);
			ShaderBlob* ps_blob = g_ShaderSystem->GetShaderBlob(PS_VertexColor);

			D3D12_ROOT_SIGNATURE_DESC signature_desc = {};
			signature_desc.NumParameters		= 0;
			signature_desc.pParameters			= nullptr;
			signature_desc.NumStaticSamplers	= 0;
			signature_desc.pStaticSamplers		= nullptr;
			signature_desc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			GPtr<ID3DBlob> root_signature_blob;
			GPtr<ID3DBlob> error_blob;
			RB_ASSERT_FATAL_RELEASE_D3D(D3D12SerializeRootSignature(&signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &error_blob), "Could not serialize root signature");

			RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&_RootSignature)), "Could not create root signature");

			D3D12_BLEND_DESC blend_desc = {};
			blend_desc.AlphaToCoverageEnable					= false;
			blend_desc.IndependentBlendEnable					= false;
			blend_desc.RenderTarget[0].BlendEnable				= false;
			blend_desc.RenderTarget[0].LogicOpEnable			= false;
			blend_desc.RenderTarget[0].RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;

			D3D12_INPUT_ELEMENT_DESC input_elements[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0  },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			};		

			D3D12_RASTERIZER_DESC ras_desc = {};
			ras_desc.FillMode				= D3D12_FILL_MODE_SOLID;
			ras_desc.CullMode				= D3D12_CULL_MODE_NONE;
			ras_desc.DepthClipEnable		= FALSE;
			ras_desc.FrontCounterClockwise	= FALSE;
			ras_desc.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
			ras_desc.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			ras_desc.SlopeScaledDepthBias	= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			ras_desc.MultisampleEnable		= FALSE;
			ras_desc.AntialiasedLineEnable	= FALSE;
			ras_desc.ForcedSampleCount		= 0;
			ras_desc.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			D3D12_DEPTH_STENCIL_DESC ds_desc = {};
			ds_desc.DepthEnable				= FALSE;
			ds_desc.DepthWriteMask			= D3D12_DEPTH_WRITE_MASK_ZERO;
			ds_desc.DepthFunc				= D3D12_COMPARISON_FUNC_ALWAYS;
			ds_desc.StencilEnable			= FALSE;
			ds_desc.StencilReadMask			= 0;
			ds_desc.StencilWriteMask		= 0;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
			pso_desc.pRootSignature			= _RootSignature.Get();
			pso_desc.VS						= { vs_blob->m_ShaderBlob, vs_blob->m_ShaderBlobSize }; //{ vs->GetBufferPointer(), vs->GetBufferSize() };
			pso_desc.PS						= { ps_blob->m_ShaderBlob, ps_blob->m_ShaderBlobSize }; //{ ps->GetBufferPointer(), ps->GetBufferSize() };
			//pso_desc.StreamOutput			= ;
			pso_desc.BlendState				= blend_desc;
			pso_desc.SampleMask				= UINT_MAX;
			pso_desc.RasterizerState		= ras_desc;
			pso_desc.DepthStencilState		= ds_desc;
			pso_desc.InputLayout			= { input_elements, 2 };
			//pso_desc.IBStripCutValue		= D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			pso_desc.PrimitiveTopologyType	= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			pso_desc.NumRenderTargets		= 1;
			pso_desc.RTVFormats[0]			= GetPrimaryWindow()->GetSwapChain()->GetBackBufferFormat();
			pso_desc.DSVFormat				= DXGI_FORMAT_UNKNOWN;
			pso_desc.SampleDesc				= { 1, 0 };
			pso_desc.NodeMask				= 0;
			//pso_desc.CachedPSO			= NULL;
			pso_desc.Flags					= D3D12_PIPELINE_STATE_FLAG_NONE;

			_Pso = g_PipelineManager->GetGraphicsPipeline(pso_desc);
		}

		// VAO
		{
			_VertexRes = g_ResourceManager->CreateCommittedResource(L"Vertex resource", CD3DX12_RESOURCE_DESC::Buffer(sizeof(_VertexData)), D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
			_UploadResource = g_ResourceManager->CreateCommittedResource(L"Vertex upload resource", CD3DX12_RESOURCE_DESC::Buffer(sizeof(_VertexData)), D3D12_HEAP_TYPE_UPLOAD, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ);

			float* upload_memory;
			_UploadResource->Map(0, nullptr, reinterpret_cast<void**>(&upload_memory));
			memcpy(upload_memory, _VertexData, sizeof(_VertexData));
			_UploadResource->Unmap(0, nullptr);

			CommandList* command_list = _GraphicsQueue->GetCommandList();
			ID3D12GraphicsCommandList2* d3d_list = command_list->GetCommandList();

			d3d_list->CopyResource(_VertexRes.Get(), _UploadResource.Get());

			uint64_t fence_value = _GraphicsQueue->ExecuteCommandList(command_list);
			_GraphicsQueue->WaitForFenceValue(fence_value);

			_VaoView.BufferLocation = _VertexRes->GetGPUVirtualAddress();
			_VaoView.SizeInBytes	= sizeof(_VertexData);
			_VaoView.StrideInBytes	= sizeof(float) * 5;
		}

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

			// Poll inputs and update windows
			for (Graphics::Window* window : m_Windows)
			{
				if (window->HasWindow())
				{
					window->Update();
				}
				else
				{
					m_CheckWindows = true;
				}
			}

			// Check if there are any windows that should be closed/removed
			if (m_CheckWindows)
			{
				for (int i = 0; i < m_Windows.size(); i++)
				{
					if (m_Windows[i]->ShouldClose() || !m_Windows[i]->HasWindow())
					{
						delete m_Windows[i];
						m_Windows.erase(m_Windows.begin() + i);
						i--;
					}
				}

				if (m_Windows.size() == 0)
				{
					RB_LOG(LOGTAG_EVENT, "Last window has been closed, requesting to stop application");
					m_ShouldStop = true;
				}

				m_CheckWindows = false;
			}

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

		_GraphicsQueue->WaitUntilEmpty();

		delete g_PipelineManager;
		delete g_ShaderSystem;
		delete g_ResourceStateManager;
		delete g_ResourceManager;

		for (int i = 0; i < m_Windows.size(); i++)
		{
			delete m_Windows[i];
		}
		m_Windows.clear();

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
		Graphics::Window* window = GetPrimaryWindow();

		if (window->IsMinimized())
		{
			return;
		}

		CommandList* command_list = _GraphicsQueue->GetCommandList();
		ID3D12GraphicsCommandList2* d3d_list = command_list->GetCommandList();

		auto back_buffer = window->GetSwapChain()->GetCurrentBackBuffer();

		{
			// Because we directly want to use the backbuffer, first make sure it is not being used by a previous frame anymore on the GPU by waiting on the fence
			uint64_t value = window->GetSwapChain()->GetCurrentBackBufferIndex();
			_GraphicsQueue->WaitForFenceValue(_FenceValues[value]);

			// Clear the render target
			{
				RB_PROFILE_GPU_SCOPED(d3d_list, "Clear");

				CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					back_buffer.Get(),
					D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
				);

				d3d_list->ResourceBarrier(1, &barrier);

				FLOAT clear_color[] = { _Value, 0.1f, 0.1f, 0.0f };

				d3d_list->ClearRenderTargetView(window->GetSwapChain()->GetCurrentDescriptorHandleCPU(), clear_color, 0, nullptr);
			}

			// Draw
			{
				RB_PROFILE_GPU_SCOPED(d3d_list, "Draw");

				d3d_list->SetPipelineState(_Pso.Get());
				d3d_list->SetGraphicsRootSignature(_RootSignature.Get());

				// Input assembly
				d3d_list->IASetVertexBuffers(0, 1, &_VaoView);
				d3d_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				
				// Rasterizer state
				d3d_list->RSSetScissorRects(1, &CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX));
				d3d_list->RSSetViewports(1, &CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight())));

				// Output merger
				d3d_list->OMSetRenderTargets(1, &window->GetSwapChain()->GetCurrentDescriptorHandleCPU(), true, nullptr);

				d3d_list->DrawInstanced(sizeof(_VertexData) / (sizeof(float) * 5), 1, 0, 0);
			}

			// Present
			{
				//RB_PROFILE_GPU_SCOPED(d3d_list, "Present");

				CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					back_buffer.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
				);

				d3d_list->ResourceBarrier(1, &barrier);

				uint64_t value = window->GetSwapChain()->GetCurrentBackBufferIndex();
				_FenceValues[value] = _GraphicsQueue->ExecuteCommandList(command_list);

				window->GetSwapChain()->Present(VsyncMode::On);
			}

		}
	}

	void Application::FinishRenderFrame()
	{

	}

	Graphics::Window* Application::GetPrimaryWindow() const
	{
		// TODO, should probably change this and being able to set primary windows
		return m_Windows[0];
	}

	Graphics::Window* Application::GetWindow(uint32_t index) const
	{
		RB_ASSERT_FATAL(LOGTAG_MAIN, index < m_Windows.size() && index >= 0, "Trying to get a window that does not exist");

		return m_Windows[index];
	}

	Graphics::Window* Application::FindWindow(void* window_handle) const
	{
		for (Graphics::Window* window : m_Windows)
		{
			if (window->IsSameWindow(window_handle))
			{
				return window;
			}
		}

		RB_LOG_WARN(LOGTAG_MAIN, "Could not find the window associated to the window handle");

		return nullptr;
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
				for (Graphics::Window* window : m_Windows)
				{
					if (window->HasWindow() && window->InFocus())
					{
						window->Resize(200, 200, 200, 100);
					}
				}
			}
		}, event);

		// BindEvent<EventType>(RB_BIND_EVENT_FN(Class::Method), event);

		BindEvent<WindowCloseRequestEvent>([this](WindowCloseRequestEvent& close_event)
		{
			m_CheckWindows = true;
		}, event);
	}
}