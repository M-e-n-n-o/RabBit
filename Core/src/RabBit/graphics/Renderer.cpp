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
	void RenderJob(JobData* data);
	void EventJob(JobData* data);

	struct RenderContext : public JobData
	{
		RenderPass**					renderPasses;
		RenderPassEntry**				renderPassEntries;
		uint32_t						totalPasses;
								 
		RenderInterface*				graphicsInterface;
										
		uint64_t*						renderFrameIndex;
		CRITICAL_SECTION*				renderFrameIndexCS;
										
		std::function<void()>			OnRenderFrameStart;
		std::function<void()>			OnRenderFrameEnd;
		std::function<void()>			SyncWithGpu;

		~RenderContext()
		{
			for (int i = 0; i < totalPasses; ++i)
			{
				SAFE_DELETE(renderPassEntries[i]);
			}
			SAFE_FREE(renderPassEntries);
		}
	};

	struct EventContext : public JobData
	{
		std::function<void()> ProcessEvents;
	};

	Renderer::Renderer()
		: EventListener(kEventCat_Window)
		, m_IsShutdown(true)
	{
	}

	void Renderer::Init()
	{
		m_IsShutdown = false;

		m_CopyInterface		= RenderInterface::Create(true);
		m_GraphicsInterface = RenderInterface::Create(false);

		m_RenderThread = new WorkerThread(L"Render Thread", ThreadPriority::High);

		m_RenderJobType = m_RenderThread->AddJobType(&RenderJob, true);
		m_EventJobType  = m_RenderThread->AddJobType(&EventJob,  true);

		m_RenderFrameIndex = 0;
		InitializeCriticalSection(&m_RenderFrameIndexCS);

		// Set the render passes
		m_TotalPasses = 1;
		m_RenderPasses = (RenderPass**) ALLOC_HEAP(sizeof(RenderPass*) * m_TotalPasses);
		m_RenderPasses[0] = new GBufferPass();
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

		// Blocks until render thread is completely idle
		delete m_RenderThread;

		SyncWithGpu();

		for (int i = 0; i < m_TotalPasses; ++i)
		{
			delete m_RenderPasses[i];
		}
		SAFE_FREE(m_RenderPasses);

		DeleteCriticalSection(&m_RenderFrameIndexCS);

		delete m_GraphicsInterface;
		delete m_CopyInterface;
	}

	void Renderer::SubmitFrameContext(const Entity::Scene* const scene)
	{
		RenderPassEntry** entries = (RenderPassEntry**) ALLOC_HEAP(sizeof(RenderPassEntry*) * m_TotalPasses);

		// Gather the entries from all render passes
		for (int i = 0; i < m_TotalPasses; i++)
		{
			entries[i] = m_RenderPasses[i]->SubmitEntry(nullptr, scene);
		}

		// Schedule a render job (overwrites the previous render job if not yet picked up)
		{
			RenderContext* context = new RenderContext();
			context->renderPasses		= m_RenderPasses;
			context->renderPassEntries	= entries;
			context->totalPasses		= m_TotalPasses;
			context->graphicsInterface	= m_GraphicsInterface;
			context->renderFrameIndex	= &m_RenderFrameIndex;
			context->renderFrameIndexCS = &m_RenderFrameIndexCS;
			context->OnRenderFrameStart	= std::bind(&Renderer::OnFrameStart, this);
			context->OnRenderFrameEnd	= std::bind(&Renderer::OnFrameEnd, this);
			context->SyncWithGpu		= std::bind(&Renderer::SyncWithGpu, this);

			m_RenderThread->ScheduleJob(m_RenderJobType, context);
		}

		// Schedule an event handling job
		{
			EventContext* context = new EventContext();
			context->ProcessEvents = std::bind(&Renderer::ProcessEvents, this);

			m_RenderThread->ScheduleJob(m_EventJobType, context);
		}

		// Stream some resources on the main thread (maybe in the future do this on a different thread?)
		{
			StreamResources(scene);
		}

		// Sync with the render thread on a stall
		JobID stalling_job;
		if (m_RenderThread->IsStalling(m_RenderThreadTimeoutMs, stalling_job))
		{
			RB_LOG(LOGTAG_GRAPHICS, "Detected stall in render thread, syncing...");
			SyncRenderer(false);
		}
	}

	void Renderer::SyncRenderer(bool gpu_sync)
	{
		m_RenderThread->SyncAll();

		if (gpu_sync)
		{
			SyncWithGpu();
		}
	}

	uint64_t Renderer::GetRenderFrameIndex()
	{
		EnterCriticalSection(&m_RenderFrameIndexCS);
		uint64_t index = m_RenderFrameIndex;
		LeaveCriticalSection(&m_RenderFrameIndexCS);

		return index;
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

	void Renderer::StreamResources(const Entity::Scene* const scene)
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

		static List<const Entity::Mesh*> already_uploaded;

		List<const Entity::ObjectComponent*> meshes = scene->GetComponentsWithTypeOf<Entity::Mesh>();

		uint32_t uploaded = 0;

		for (const Entity::ObjectComponent* obj : meshes)
		{
			const Entity::Mesh* mesh = (const Entity::Mesh*)obj;

			if (std::find(already_uploaded.begin(), already_uploaded.end(), mesh) != already_uploaded.end())
			{
				continue;
			}

			m_CopyInterface->UploadDataToResource(mesh->GetVertexBuffer(), mesh->GetVertexBuffer()->GetData(), mesh->GetVertexBuffer()->GetSize());
			already_uploaded.push_back(mesh);
			uploaded++;
		}

		if (uploaded > 0)
		{
			// Make sure the graphics interface waits until the streaming has been completed before starting to render
			Shared<RIExecutionGuard> guard = m_CopyInterface->ExecuteOnGpu();
			m_GraphicsInterface->GpuWaitOn(guard.get());
		}
	}

	void EventJob(JobData* data)
	{
		EventContext* context = (EventContext*) data;
		context->ProcessEvents();
	}

	// TODO Remove!
	Shared<RIExecutionGuard> _FenceValues[Graphics::Window::BACK_BUFFER_COUNT] = {};

	void RenderJob(JobData* data)
	{
		RenderContext* context = (RenderContext*) data;

		context->OnRenderFrameStart();

		Graphics::Window* window = Application::GetInstance()->GetPrimaryWindow();

		if (window && window->IsValid() && !window->IsMinimized())
		{
			GPtr<ID3D12GraphicsCommandList2> command_list = ((D3D12::RenderInterfaceD3D12*)context->graphicsInterface)->GetCommandList();

			Texture2D* back_buffer = window->GetCurrentBackBuffer();

			// Render frame
			{
				RB_PROFILE_GPU_SCOPED(command_list.Get(), "Frame");

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

				// Render the different passes
				for (int i = 0; i < context->totalPasses; ++i)
				{
					RenderPassEntry* entry = context->renderPassEntries[i];

					if (entry == nullptr)
					{
						continue;
					}

					RB_PROFILE_GPU_SCOPED(command_list.Get(), context->renderPasses[i]->GetConfiguration().friendlyName);

					context->renderPasses[i]->Render(context->graphicsInterface, nullptr, context->renderPassEntries[i], (RenderResource**) &back_buffer, nullptr, nullptr);

					context->graphicsInterface->InvalidateState();
				}
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

		context->OnRenderFrameEnd();

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
	}
}