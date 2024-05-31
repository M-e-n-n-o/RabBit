#include "Renderer.h"
#include "RabBitCommon.h"
#include "Renderer.h"
#include "RenderInterface.h"
#include "RenderPass.h"
#include "app/Application.h"
#include "graphics/Window.h"
#include "input/events/WindowEvent.h"

#include "entity/Scene.h"
#include "d3d12/RendererD3D12.h"

#include <atomic>

#include "passes/GBuffer.h"

#include "entity/components/Mesh.h"
#include "d3d12/GraphicsDevice.h"
#include "d3d12/DeviceQueue.h"
#include "RenderInterface.h"
#include "d3d12/resource/GpuResource.h"
#include "d3d12/resource/RenderResourceD3D12.h"
#include "d3d12/GraphicsDevice.h"
#include "d3d12/RenderInterfaceD3D12.h"

using namespace RB::Input::Events;

namespace RB::Graphics
{
	DWORD WINAPI RenderLoop(PVOID param);
	DWORD WINAPI ResourceStreamLoop(PVOID param);

	Renderer::Renderer()
		: EventListener(kEventCat_Window)
		, m_IsShutdown(true)
	{
	}

	void Renderer::Init()
	{
		m_IsShutdown = false;

		m_SharedContext	= new SharedRenderThreadContext();
		m_SharedContext->graphicsInterface	= RenderInterface::Create(false);
		m_SharedContext->copyInterface		= RenderInterface::Create(true);
		m_SharedContext->renderState		= ThreadState::Idle;
		m_SharedContext->OnRenderFrameStart = std::bind(&Renderer::OnFrameStart, this);
		m_SharedContext->OnRenderFrameEnd	= std::bind(&Renderer::OnFrameEnd, this);
		m_SharedContext->ProcessEvents		= std::bind(&Renderer::ProcessEvents, this);
		m_SharedContext->SyncWithGpu		= std::bind(&Renderer::SyncWithGpu, this);
		m_SharedContext->renderFrameIndex	= 0;

		for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
		{
			m_SharedContext->renderTasks[task_type] = {};
			m_SharedContext->renderTasks[task_type].data = nullptr;
			m_SharedContext->renderTasks[task_type].dataSize = 0;
		}

		LARGE_INTEGER li;
		if (!QueryPerformanceFrequency(&li))
		{
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Could not retrieve value from QueryPerformanceFrequency");
		}

		m_SharedContext->performanceFreqMs = double(li.QuadPart) / 1000.0;

		// Set the render passes
		m_SharedContext->totalPasses = 1;
		m_SharedContext->renderPasses = (RenderPass**) ALLOC_HEAP(sizeof(RenderPass*) * m_SharedContext->totalPasses);
		m_SharedContext->renderPasses[0] = new GBufferPass();

		// Spawn the thread
		DWORD id;
		m_RenderThread = CreateThread(NULL, 0, RenderLoop, (PVOID)m_SharedContext, 0, &id);
		RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, m_RenderThread != 0, "Failed to create render thread");
		SetThreadDescription(m_RenderThread, L"Render Thread");
		SetThreadPriority(m_RenderThread, THREAD_PRIORITY_HIGHEST);
	}

	Renderer::~Renderer()
	{
		if (!m_IsShutdown)
		{
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Deleting renderer before shutting it down!");
		}
	}

	void Renderer::Shutdown()
	{
		m_IsShutdown = true;

		SendRenderThreadTask(RenderThreadTaskType::Stop, GetDummyTask());

		WaitForMultipleObjects(1, &m_RenderThread, TRUE, INFINITE);
		CloseHandle(m_RenderThread);

		for (int i = 0; i < m_SharedContext->totalPasses; ++i)
		{
			delete m_SharedContext->renderPasses[i];
		}
		SAFE_FREE(m_SharedContext->renderPasses);

		delete m_SharedContext->graphicsInterface;
		delete m_SharedContext->copyInterface;
		delete m_SharedContext;
	}

	void Renderer::SubmitFrameContext(const Entity::Scene* const scene)
	{
		uint64_t data_size = sizeof(RenderPassContext) * m_SharedContext->totalPasses;

		RenderPassContext* contexts = (RenderPassContext*)ALLOC_HEAP(data_size);

		for (int i = 0; i < m_SharedContext->totalPasses; ++i)
		{
			contexts[i] = m_SharedContext->renderPasses[i]->SubmitContext(nullptr, scene);
		}

		RenderTask data;
		data.hasTask	= true;
		data.data		= contexts;
		data.dataSize	= data_size;

		SendRenderThreadTask(RenderThreadTaskType::RenderFrame, data);

		// Also submit a new request to handle new window events
		SendRenderThreadTask(RenderThreadTaskType::HandleEvents, GetDummyTask());

		// Sync with the renderer if it is stalling
		{
			EnterCriticalSection(&m_SharedContext->renderKickCS);
			ThreadState render_state  = m_SharedContext->renderState;
			uint64_t	counter_start = m_SharedContext->renderCounterStart;
			LeaveCriticalSection(&m_SharedContext->renderKickCS);

			if (render_state != ThreadState::Idle)
			{
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);

				if ((double(li.QuadPart - counter_start) / m_SharedContext->performanceFreqMs) > m_SharedContext->renderThreadTimeoutMs)
				{
					RB_LOG(LOGTAG_GRAPHICS, "Detected stall in render thread, syncing...");
					SyncRenderer(false);
				}
			}
		}
	}

	void Renderer::SyncRenderer(bool gpu_sync)
	{
		// Sync with render thread
		EnterCriticalSection(&m_SharedContext->renderSyncCS);

		while (m_SharedContext->renderState != ThreadState::Idle)
		{
			SleepConditionVariableCS(&m_SharedContext->renderSyncCV, &m_SharedContext->renderSyncCS, INFINITE);
		}

		LeaveCriticalSection(&m_SharedContext->renderSyncCS);

		if (gpu_sync)
		{
			SyncWithGpu();
		}
	}

	uint64_t Renderer::GetRenderFrameIndex() const
	{
		EnterCriticalSection(&m_SharedContext->renderFrameIndexCS);
		uint64_t index = m_SharedContext->renderFrameIndex;
		LeaveCriticalSection(&m_SharedContext->renderFrameIndexCS);

		return index;
	}

	void Renderer::SendRenderThreadTask(RenderThreadTaskType task_type, const RenderTask& task_data)
	{
		EnterCriticalSection(&m_SharedContext->renderKickCS);

		RenderTask& task = m_SharedContext->renderTasks[task_type];

		// If there was a previous task here already with some data, delete and overwrite
		if (task.hasTask && task.dataSize > 0)
		{
			SAFE_FREE(task.data);
		}

		m_SharedContext->renderTasks[task_type] = task_data;
		LeaveCriticalSection(&m_SharedContext->renderKickCS);

		// Kick render thread
		WakeConditionVariable(&m_SharedContext->renderKickCV);
	}

	void Renderer::OnEvent(Event& event)
	{
		WindowEvent* window_event = static_cast<WindowEvent*>(&event);

		Window* window = Application::GetInstance()->FindWindow(window_event->GetWindowHandle());

		if (window)
		{
			window->ProcessEvent(*window_event);
		}
	}

	Renderer::RenderTask Renderer::GetDummyTask()
	{
		RenderTask dummy = {};
		dummy.hasTask	= true;
		dummy.data		= nullptr;
		dummy.dataSize	= 0;

		return dummy;
	}

	Renderer* Renderer::Create(bool enable_validation_layer)
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
			return new D3D12::RendererD3D12(enable_validation_layer);
		default:
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Did not yet implement the Renderer for the set graphics API");
			break;
		}

		return nullptr;
	}

	DWORD WINAPI RenderLoop(PVOID param)
	{
		Renderer::SharedRenderThreadContext* context = (Renderer::SharedRenderThreadContext*)param;

		InitializeConditionVariable(&context->renderKickCV);
		InitializeCriticalSection(&context->renderKickCS);
		InitializeConditionVariable(&context->renderSyncCV);
		InitializeCriticalSection(&context->renderSyncCS);
		InitializeCriticalSection(&context->renderFrameIndexCS);

		// Spawn the streaming thread
		{
			InitializeConditionVariable(&context->streamingKickCV);
			InitializeCriticalSection(&context->streamingKickCS);
			InitializeConditionVariable(&context->streamingSyncCV);
			InitializeCriticalSection(&context->streamingSyncCS);

			DWORD id;
			context->streamingThread = CreateThread(NULL, 0, ResourceStreamLoop, (PVOID)context, 0, &id);
			RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, context->streamingThread != 0, "Failed to create resource streaming thread");
			SetThreadDescription(context->streamingThread, L"Resource Streaming Thread");
			SetThreadPriority(context->streamingThread, THREAD_PRIORITY_HIGHEST);
		}



		// TODO Remove!
		Shared<RIExecutionGuard> _FenceValues[Graphics::Window::BACK_BUFFER_COUNT] = {};




		bool stop = false;
		while (!stop)
		{
			bool has_tasks[Renderer::RenderThreadTaskType::Count];
			void* render_data[Renderer::RenderThreadTaskType::Count];
			for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
			{
				has_tasks[task_type] = false;
				render_data[task_type] = nullptr;
			}

			// Prepare
			{
				EnterCriticalSection(&context->renderKickCS);

				// Reset timer
				context->renderCounterStart = 0;

				// Wait until a new task is available
				while (true)
				{
					bool has_new_task = false;
					for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
					{
						if (context->renderTasks[task_type].hasTask)
						{
							has_tasks[task_type] = true;
							has_new_task = true;
						}
					}

					if (has_new_task)
					{
						break;
					}

					// Notify the sync that we are starting to idle
					{
						EnterCriticalSection(&context->renderSyncCS);
						context->renderState = Renderer::ThreadState::Idle;
						LeaveCriticalSection(&context->renderSyncCS);
						WakeConditionVariable(&context->renderSyncCV);
					}

					// Sleep
					SleepConditionVariableCS(&context->renderKickCV, &context->renderKickCS, INFINITE);
				}

				context->renderState = Renderer::ThreadState::Running;

				for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
				{
					// Copy the task data if there is any
					if (context->renderTasks[task_type].dataSize > 0)
					{
						render_data[task_type] = ALLOC_HEAP(context->renderTasks[task_type].dataSize);

						memcpy(render_data[task_type], context->renderTasks[task_type].data, context->renderTasks[task_type].dataSize);
					}

					// Reset the render task
					context->renderTasks[task_type].hasTask = false;
					SAFE_FREE(context->renderTasks[task_type].data);
					context->renderTasks[task_type].dataSize = 0;
				}

				// Start timer
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				context->renderCounterStart = li.QuadPart;

				LeaveCriticalSection(&context->renderKickCS);
			}

			// Execute the tasks
			for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
			{
				if (!has_tasks[task_type])
				{
					continue;
				}

				switch (task_type)
				{
				case Renderer::RenderThreadTaskType::Stop:
				{
					stop = true;

					// Also stop streaming thread
					{
						EnterCriticalSection(&context->streamingKickCS);
						context->streamingState = Renderer::ThreadState::Running;
						context->sharedRenderStreamingData = nullptr;
						LeaveCriticalSection(&context->streamingKickCS);
						WakeConditionVariable(&context->streamingKickCV);
					}
				}
				break;

				case Renderer::RenderThreadTaskType::HandleEvents:
				{
					context->ProcessEvents();
				}
				break;

				case Renderer::RenderThreadTaskType::RenderFrame:
				{
					context->OnRenderFrameStart();

					// Kick off streaming thread
					{
						EnterCriticalSection(&context->streamingKickCS);
						context->streamingState = Renderer::ThreadState::Running;
						context->sharedRenderStreamingData = render_data[task_type];
						LeaveCriticalSection(&context->streamingKickCS);
						WakeConditionVariable(&context->streamingKickCV);
					}

					Graphics::Window* window = Application::GetInstance()->GetPrimaryWindow();

					if (window && window->IsValid() && !window->IsMinimized())
					{
						GPtr<ID3D12GraphicsCommandList2> command_list = ((D3D12::RenderInterfaceD3D12*)context->graphicsInterface)->GetCommandList();

						Texture2D* back_buffer = window->GetCurrentBackBuffer();

						uint64_t value = window->GetCurrentBackBufferIndex();
						if (_FenceValues[value])
						{
							_FenceValues[value]->WaitUntilFinishedRendering();
						}

						// Clear the backbuffer
						{
							RB_PROFILE_GPU_SCOPED(command_list.Get(), "Clear");

							context->graphicsInterface->Clear(back_buffer, { 0.0f, 0.1f, 0.1f, 0.0f });
						}

						RenderPassContext* contexts = (RenderPassContext*)render_data[task_type];

						// Render the different passes
						for (int i = 0; i < context->totalPasses; ++i)
						{
							RB_PROFILE_GPU_SCOPED(command_list.Get(), "Pass");

							context->renderPasses[i]->Render(context->graphicsInterface, nullptr, &contexts[i], (RenderResource**) &back_buffer, nullptr, nullptr);

							context->graphicsInterface->InvalidateState();
						}

						// Present
						{
							//RB_PROFILE_GPU_SCOPED(d3d_list, "Present");

							CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
								((D3D12::GpuResource*)back_buffer->GetNativeResource())->GetResource().Get(),
								D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
							);

							((D3D12::GpuResource*)back_buffer->GetNativeResource())->UpdateState(D3D12_RESOURCE_STATE_PRESENT);

							command_list->ResourceBarrier(1, &barrier);

							uint64_t value = window->GetCurrentBackBufferIndex();
							_FenceValues[value] = context->graphicsInterface->ExecuteOnGpu();

							window->Present(VsyncMode::On);
						}
					}

					// Present

					//{
					//	Graphics::Window* window = Application::GetInstance()->GetPrimaryWindow();

					//	if (window && window->IsValid() && !window->IsMinimized())
					//	{
					//		auto back_buffer = window->GetCurrentBackBuffer();

					//		GPtr<ID3D12GraphicsCommandList2> command_list = ((D3D12::RenderInterfaceD3D12*)context->graphicsInterface)->GetCommandList();

					//		// Because we directly want to use the backbuffer, first make sure it is not being used by a previous frame anymore on the GPU by waiting on the fence
					//		uint64_t value = window->GetCurrentBackBufferIndex();
					//		//D3D12::g_GraphicsDevice->GetGraphicsQueue()->CpuWaitForFenceValue(_FenceValues[value]);
					//		if (_FenceValues[value])
					//		{
					//			_FenceValues[value]->WaitUntilFinishedRendering();
					//		}


					//		// Clear the render target
					//		{
					//			RB_PROFILE_GPU_SCOPED(command_list.Get(), "Clear");

					//			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					//				((D3D12::GpuResource*)back_buffer->GetNativeResource())->GetResource().Get(),
					//				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
					//			);

					//			command_list->ResourceBarrier(1, &barrier);

					//			FLOAT clear_color[] = { 0.0f, 0.1f, 0.1f, 0.0f };

					//			command_list->ClearRenderTargetView(*((D3D12::Texture2DD3D12*)back_buffer)->GetCpuHandle(), clear_color, 0, nullptr);
					//		}

					//		// Draw
					//		//{
					//		//	RB_PROFILE_GPU_SCOPED(command_list.Get(), "Draw");

					//		//	command_list->SetPipelineState(_Pso.Get());
					//		//	command_list->SetGraphicsRootSignature(_RootSignature.Get());

					//		//	// Input assembly
					//		//	command_list->IASetVertexBuffers(0, 1, &_VaoView);
					//		//	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					//		//	// Rasterizer state
					//		//	command_list->RSSetScissorRects(1, &CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX));
					//			//command_list->RSSetViewports(1, &CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight())));

					//		//	// Output merger
					//		//	command_list->OMSetRenderTargets(1, &((Texture2DD3D12*)back_buffer)->GetCpuHandle(), true, nullptr);

					//		//	command_list->DrawInstanced(sizeof(_VertexData) / (sizeof(float) * 5), 1, 0, 0);
					//		//}

					//		// Present
					//		{
					//			//RB_PROFILE_GPU_SCOPED(d3d_list, "Present");

					//			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					//				((D3D12::GpuResource*)back_buffer->GetNativeResource())->GetResource().Get(),
					//				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
					//			);

					//			command_list->ResourceBarrier(1, &barrier);

					//			uint64_t value = window->GetCurrentBackBufferIndex();
					//			//_FenceValues[value] = D3D12::g_GraphicsDevice->GetGraphicsQueue()->ExecuteCommandList(command_list);
					//			_FenceValues[value] = context->graphicsInterface->ExecuteOnGpu();

					//			window->Present(VsyncMode::On);
					//		}

					//	}
					//}

					/*
						Rendering flow:
						(command list 0, 1 & 2 are from frame i-2, 3, 4 & 5 are from frame i-1)

						Firstly, we start filling command list 6 with game rendering. Before doing the executeCommandList on this,
						we place a GPU barrier on the previous' frame commandlist 4 that contains upscaling, post and UI rendering
						(as game, upscaling, post and UI rendering shares some resources).

						Secondly, we start filling command list 7 with upscaling, if enabled, post and UI rendering. Before doing the executeCommandList on this,
						we place a GPU barrier on the current' frame commandlist 6, that contains game rendering. This commandlist finally outputs on the game backbuffer.

						Thirdly, we start filling command list 8 with copying to the actual backbuffer (+ optional letterboxing). Before doing the executeCommandList on this,
						we place a GPU barrier on the current' frame commandlist 7, that contains upscaling, post and UI rendering,
						but we also place a CPU barrier on frame i-2' commandlist 2 that waits until the current backbuffer is available again
						(commandlist 2 contains the copy to backbuffer command list of the backbuffer that we want to use here again, because we have 2 backbuffers).

						MAYBE INSTEAD OF ALWAYS SEPERATING PRE-UPSCALER AND POST-UPSCALER IN A SEPARATE COMMAND LIST,
						JUST DO AN EXECUTE AFTER EVERY VIEWCONTEXT (AFTER UI RENDERING) + INTERMEDIATE EXECUTES AFTER EVERY X AMOUNT OF DRAW CALLS?

						BUT HOW TO DO SYNC POINTS WITH MULTIPLE VIEW CONTEXTS??
							- Do all offscreen contexts first (only game, upscaling, post and UI rendering, so no backbuffer)
							- Then do the main context

						When we need to change the render settings, then we will place a CPU wait until GPU idle and only then apply
						the new settings and signal that the thread is idle again!
					*/

					// Sync with streaming thread
					{
						EnterCriticalSection(&context->streamingSyncCS);

						while (context->streamingState != Renderer::ThreadState::Idle)
						{
							SleepConditionVariableCS(&context->streamingSyncCV, &context->streamingSyncCS, INFINITE);
						}

						context->sharedRenderStreamingData = nullptr;

						LeaveCriticalSection(&context->streamingSyncCS);
					}

					context->OnRenderFrameEnd();

					// Update the frame index
					{
						EnterCriticalSection(&context->renderFrameIndexCS);
						++context->renderFrameIndex;
						LeaveCriticalSection(&context->renderFrameIndexCS);
					}
				}
				break;

				default:
					RB_LOG_WARN(LOGTAG_GRAPHICS, "Render task type not yet implemented");
					break;
				}

				SAFE_FREE(render_data[task_type]);
			}

			if (stop)
			{
				break;
			}
		}

		// Wait for resource streaming thread
		WaitForMultipleObjects(1, &context->streamingThread, TRUE, INFINITE);
		CloseHandle(context->streamingThread);

		RB_LOG(LOGTAG_GRAPHICS, "Render thread terminating");

		context->SyncWithGpu();

		DeleteCriticalSection(&context->streamingKickCS);
		DeleteCriticalSection(&context->streamingSyncCS);

		DeleteCriticalSection(&context->renderKickCS);
		DeleteCriticalSection(&context->renderSyncCS);
		DeleteCriticalSection(&context->renderFrameIndexCS);

		context->renderState = Renderer::ThreadState::Terminated;

		return 0;
	}

	DWORD WINAPI ResourceStreamLoop(PVOID param)
	{
		Renderer::SharedRenderThreadContext* context = (Renderer::SharedRenderThreadContext*)param;

		context->streamingState = Renderer::ThreadState::Idle;

		while (true)
		{
			// Wait until kick from render thread
			{
				EnterCriticalSection(&context->streamingKickCS);

				while (context->streamingState == Renderer::ThreadState::Idle)
				{
					SleepConditionVariableCS(&context->streamingKickCV, &context->streamingKickCS, INFINITE);
				}

				LeaveCriticalSection(&context->streamingKickCS);
			}

			if (context->sharedRenderStreamingData == nullptr)
			{
				// Terminate
				break;
			}

			// Do resource management/streaming
			{
				/*
					!!!!! RESOURCE STREAMING THREAD SHOULD ONLY SCHEDULE THINGS ON THE COPY INTERFACE, NO GRAPHICS STUFF !!!!!! 

					(Slowly) Upload needed resources for next frame, like new vertex data or textures.
					Constantly check if the main thread is waiting until this task is done, if so, finish uploading only the most crucial resources 
					and then stop (so textures are not completely crucial, as we can use the lower mips as fallback, or even white textures).
					Maybe an idea to give this task always a minimum amount of time to let it upload resources, because when maybe the main thread
					is not doing a lot, this task will not get a lot of time actually uploading stuff.

					Make sure to put a GPU wait after uploading the last resource of the task!
				*/


				static_assert(false);
				/*
					HOE GAAN WE NU ALLEEN DE VERTEX DATA UPLOADEN???					
				
					- Eigenlijkt wil je de resource streaming kicken wanneeer SubmitFrameContext wordt aangeroepen (dus niet meer verbonden aan render thread
					- Dan gaat de main thread gewoon langs alle RenderPasses om de contexten te submitten
					- En dan kan de resource streaming alle benodigde dingen gaan doen (en krijgt het van de main thread de Scene)
					- En dan synced de main thread met de streaming thread en gaat het pas naar het volgende frame als de streaming klaar is (op de CPU alleen natuurlijk)
					- Dan ook alleen nog ff nadenken over hoeveel de streaming thread in 1x mag gaan doen? Moet het gelijk stoppen als de main thread op een sync wacht? Moet het een minimum aantal sowieso doen?

					De resource streaming zal misschien wel in een andere klasse moeten gaan om het netjes te houden
				*/


				//case Renderer::RenderThreadTaskType::ResourceManagement:
				//{
				//	const Entity::Scene* const scene = (Entity::Scene*)r->m_RenderTaskData;

				//	static List<const Entity::Mesh*> already_uploaded;

				//	List<const Entity::ObjectComponent*> meshes = scene->GetComponentsWithTypeOf<Entity::Mesh>();

				//	for (const Entity::ObjectComponent* obj : meshes)
				//	{
				//		const Entity::Mesh* mesh = (const Entity::Mesh*)obj;

				//		if (std::find(already_uploaded.begin(), already_uploaded.end(), mesh) != already_uploaded.end())
				//		{
				//			continue;
				//		}

				//		r->m_CopyInterface->UploadDataToResource(mesh->GetVertexBuffer(), mesh->GetVertexBuffer()->GetData(), mesh->GetVertexBuffer()->GetSize());
				//		already_uploaded.push_back(mesh);
				//	}

				//	Shared<RIExecutionGuard> guard = r->m_CopyInterface->ExecuteOnGpu();
				//	r->m_GraphicsInterface->GpuWaitOn(guard.get());
				//}
				//break;
			}

			// Tell render thread we are finished
			{
				EnterCriticalSection(&context->streamingSyncCS);
				context->streamingState = Renderer::ThreadState::Idle;
				LeaveCriticalSection(&context->streamingSyncCS);
				WakeConditionVariable(&context->streamingSyncCV);
			}
		}

		RB_LOG(LOGTAG_GRAPHICS, "Resource streaming thread terminating");

		context->streamingState = Renderer::ThreadState::Terminated;

		return 0;
	}
}