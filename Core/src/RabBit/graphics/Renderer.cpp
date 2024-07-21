#include "RabBitCommon.h"
#include "Renderer.h"
#include "RenderInterface.h"
#include "RenderPass.h"
#include "Window.h"
#include "ViewContext.h"
#include "ResourceDefaults.h"
#include "ResourceStreamer.h"

#include "app/Application.h"
#include "input/events/WindowEvent.h"

#include "entity/Scene.h"
#include "entity/components/Camera.h"
#include "entity/components/Transform.h"

#include "passes/GBuffer.h"

#include "d3d12/RendererD3D12.h"
// Needed for the PIX markers, need to make an abstraction for this
#include "d3d12/RenderInterfaceD3D12.h"

using namespace RB::Input::Events;

namespace RB::Graphics
{
	void RenderJob(JobData* data);
	void EventJob(JobData* data);

	struct RenderContext : public JobData
	{
		ViewContext*						viewContexts;
		uint32_t							totalViewContexts;

		List<Renderer::BackBufferGuard>*	backBufferAvailabilityGuards;

		RenderPass**						renderPasses;
		RenderPassEntry**					renderPassEntries;
		uint32_t							totalPasses;
						 
		RenderInterface*					graphicsInterface;
											
		uint64_t*							renderFrameIndex;
		CRITICAL_SECTION*					renderFrameIndexCS;
											
		std::function<void()>				OnRenderFrameStart;
		std::function<void()>				OnRenderFrameEnd;
		std::function<void()>				SyncWithGpu;

		~RenderContext()
		{
			for (int i = 0; i < totalPasses; ++i)
			{
				SAFE_DELETE(renderPassEntries[i]);
			}
			SAFE_FREE(renderPassEntries);

			SAFE_FREE(viewContexts);
		}
	};

	struct EventContext : public JobData
	{
		std::function<void()> ProcessEvents;
	};

	Renderer::Renderer(bool multi_threading_support)
		: EventListener(kEventCat_Window)
		, m_IsShutdown(true)
		, m_MultiThreadingSupport(multi_threading_support)
	{

	}

	void Renderer::Init()
	{
		m_IsShutdown = false;

		m_ResourceStreamer = new ResourceStreamer();

		// Initialize default resources
		InitResourceDefaults();

		// TODO Maybe change this to a compute queue so that it can generate mip maps
		m_CopyInterface		= RenderInterface::Create(true);
		m_GraphicsInterface = RenderInterface::Create(false);

		if (m_MultiThreadingSupport)
		{
			m_RenderThread	= new WorkerThread(L"Render Thread", ThreadPriority::High);

			m_RenderJobType = m_RenderThread->AddJobType(&RenderJob, true);
			m_EventJobType  = m_RenderThread->AddJobType(&EventJob,  true);
		}

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

		if (m_MultiThreadingSupport)
		{
			// Blocks until render thread is completely idle
			delete m_RenderThread;
		}

		SyncWithGpu();

		for (int i = 0; i < m_TotalPasses; ++i)
		{
			delete m_RenderPasses[i];
		}
		SAFE_FREE(m_RenderPasses);

		delete m_ResourceStreamer;

		DeleteCriticalSection(&m_RenderFrameIndexCS);

		// Delete default resources
		DeleteResourceDefaults();

		delete m_GraphicsInterface;
		delete m_CopyInterface;
	}

	void Renderer::SubmitFrame(const Entity::Scene* const scene)
	{
		// Schedule a render job (overwrites the previous render job if not yet picked up)
		{
			uint32_t total_view_contexts;
			ViewContext* view_contexts = CreateViewContexts(scene, total_view_contexts);

			RenderPassEntry** entries = (RenderPassEntry**)ALLOC_HEAP(sizeof(RenderPassEntry*) * m_TotalPasses);

			// Gather the entries from all render passes
			for (int i = 0; i < m_TotalPasses; i++)
			{
				entries[i] = m_RenderPasses[i]->SubmitEntry(scene);
			}

			RenderContext* context = new RenderContext();
			context->viewContexts					= view_contexts;
			context->totalViewContexts				= total_view_contexts;
			context->backBufferAvailabilityGuards	= &m_BackBufferAvailabilityGuards;
			context->renderPasses					= m_RenderPasses;
			context->renderPassEntries				= entries;
			context->totalPasses					= m_TotalPasses;
			context->graphicsInterface				= m_GraphicsInterface;
			context->renderFrameIndex				= &m_RenderFrameIndex;
			context->renderFrameIndexCS				= &m_RenderFrameIndexCS;
			context->OnRenderFrameStart				= std::bind(&Renderer::OnFrameStart, this);
			context->OnRenderFrameEnd				= std::bind(&Renderer::OnFrameEnd, this);
			context->SyncWithGpu					= std::bind(&Renderer::SyncWithGpu, this);

			if (m_MultiThreadingSupport)
			{
				m_RenderThread->ScheduleJob(m_RenderJobType, context);
			}
			else
			{
				RenderJob(context);
				delete context;
			}
		}

		// Schedule an event handling job
		{
			EventContext* context = new EventContext();
			context->ProcessEvents = std::bind(&Renderer::ProcessEvents, this);

			if (m_MultiThreadingSupport)
			{
				m_RenderThread->ScheduleJob(m_EventJobType, context);
			}
			else
			{
				EventJob(context);
				delete context;
			}
		}

		// Stream resources to the GPU on the main thread 
		// (maybe in the future do this on a different thread?)
		{
			Shared<GpuGuard> guard = m_ResourceStreamer->Stream(m_CopyInterface);

			// No need to place a GPU barrier as we do not use resources 
			// on the graphics interface that are being streamed.
			//if (guard != nullptr)
			//{
			//	// Place a GPU barrier on the graphics queue to wait on the streaming
			//	m_GraphicsInterface->GpuWaitOn(guard.get());
			//}
		}

		if (m_MultiThreadingSupport)
		{
			// Sync with the render thread on a stall
			JobID stalling_job;
			if (m_RenderThread->IsStalling(m_RenderThreadTimeoutMs, stalling_job))
			{
				RB_LOG(LOGTAG_GRAPHICS, "Detected stall in render thread, syncing...");
				SyncRenderer(false);
			}
		}
	}

	ViewContext* Renderer::CreateViewContexts(const Entity::Scene* const scene, uint32_t& out_context_count)
	{
		auto camera_components = scene->GetComponentsWithTypeOf<Entity::Camera>();

		out_context_count = camera_components.size();

		ViewContext* contexts = (ViewContext*) ALLOC_HEAP(sizeof(ViewContext) * out_context_count);

		uint32_t context_index = 0;

		uint32_t original_context_count = out_context_count;
		for (int i = 0; i < original_context_count; ++i)
		{
			const Entity::Camera*	 camera		= (const Entity::Camera*) camera_components[i];
			const Entity::Transform* transform	= camera->GetGameObject()->GetComponent<Entity::Transform>();

			if (transform == nullptr)
			{
				RB_LOG_WARN(LOGTAG_GRAPHICS, "Camera object does not have a transform, skipping...");
				out_context_count--;
				continue;
			}

			Window* window = Application::GetInstance()->GetWindow(camera->GetTargetWindowIndex());

			if (window == nullptr)
			{
				out_context_count--;
				continue;
			}

			contexts[context_index] = {};

			contexts[context_index].viewFrustum = {};
			contexts[context_index].viewFrustum.SetTransform(transform->GetLocalToWorldMatrix());
			contexts[context_index].viewFrustum.SetPerspectiveProjectionVFov(camera->GetNearPlane(), camera->GetFarPlane(), camera->GetVerticalFovInRadians(), window->GetAspectRatio(), false);
			//contexts[context_index].viewFrustum.SetOrthographicProjection(camera->GetNearPlane(), camera->GetFarPlane(), -1 * window->GetAspectRatio(), 1 * window->GetAspectRatio(), 1, -1, false);

			contexts[context_index].displayIndex = camera->GetTargetWindowIndex();

			context_index++;
		}

		return contexts;
	}

	void Renderer::SyncRenderer(bool gpu_sync)
	{
		if (m_MultiThreadingSupport)
		{
			if (!m_RenderThread->IsCurrentThread())
			{
				m_RenderThread->SyncAll();
			}
		}

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

	void EventJob(JobData* data)
	{
		EventContext* context = (EventContext*) data;
		context->ProcessEvents();
	}

	void RenderJob(JobData* data)
	{
		RenderContext* context = (RenderContext*) data;

		GPtr<ID3D12GraphicsCommandList2> command_list = ((D3D12::RenderInterfaceD3D12*)context->graphicsInterface)->GetCommandList();

		context->OnRenderFrameStart();

		{
			RB_PROFILE_GPU_SCOPED(command_list.Get(), "Frame");

			for (int view_context_index = 0; view_context_index < context->totalViewContexts; ++view_context_index)
			{
				RB_PROFILE_GPU_SCOPED(command_list.Get(), "ViewContext");

				ViewContext& view_context = context->viewContexts[view_context_index];

				Graphics::Window* window = Application::GetInstance()->GetWindow(view_context.displayIndex);

				if (!window || !window->IsValid() || window->IsMinimized())
				{
					continue;
				}

				// Wait until the backbuffer resource is available
				// (TODO Later when the passes will not use the backbuffers directly, this wait will be moved just before the backbuffer copy)
				if (view_context.displayIndex < context->backBufferAvailabilityGuards->size())
				{
					uint64_t back_buffer_index = window->GetCurrentBackBufferIndex();

					auto guards = (*context->backBufferAvailabilityGuards)[view_context.displayIndex].guards;

					if (guards[back_buffer_index])
					{
						guards[back_buffer_index]->WaitUntilFinishedRendering();
					}
				}

				Texture2D* back_buffer = window->GetCurrentBackBuffer();

				// Clear the backbuffer (TODO Create a GlobalPrepare pass to clear all necessary textures)
				{
					RB_PROFILE_GPU_SCOPED(command_list.Get(), "Clear");

					context->graphicsInterface->Clear(back_buffer, { 0.0f, 0.1f, 0.1f, 0.0f });
				}

				// Render the different passes
				for (int pass_index = 0; pass_index < context->totalPasses; ++pass_index)
				{
					RenderPassEntry* entry = context->renderPassEntries[pass_index];

					// Do not render the pass if it did not submit an entry
					if (entry == nullptr)
					{
						continue;
					}

					RB_PROFILE_GPU_SCOPED(command_list.Get(), context->renderPasses[pass_index]->GetConfiguration().friendlyName);

					context->renderPasses[pass_index]->Render(context->graphicsInterface, &view_context, context->renderPassEntries[pass_index], (RenderResource**)&back_buffer, nullptr, nullptr);
				}
			}
		}

		// Present to all windows
		for (int view_context_index = 0; view_context_index < context->totalViewContexts; ++view_context_index)
		{
			uint32_t display_index = context->viewContexts[view_context_index].displayIndex;

			Graphics::Window* window = Application::GetInstance()->GetWindow(display_index);

			if (!window || !window->IsValid() || window->IsMinimized())
			{
				continue;
			}

			Texture2D* back_buffer = window->GetCurrentBackBuffer();

			context->graphicsInterface->TransitionResource(back_buffer, ResourceState::PRESENT);
			context->graphicsInterface->FlushResourceBarriers();

			// Execute al the work to the GPU
			Shared<GpuGuard> guard = context->graphicsInterface->ExecuteOnGpu();

			if (display_index >= context->backBufferAvailabilityGuards->size())
			{
				context->backBufferAvailabilityGuards->push_back(Renderer::BackBufferGuard());
			}

			uint64_t buffer_index = window->GetCurrentBackBufferIndex();
			(*context->backBufferAvailabilityGuards)[display_index].guards[buffer_index] = guard;

			window->Present(VsyncMode::On);
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