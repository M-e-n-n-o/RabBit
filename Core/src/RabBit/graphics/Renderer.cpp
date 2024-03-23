#include "RabBitCommon.h"
#include "Renderer.h"
#include "RenderInterface.h"
#include "entity/Scene.h"
#include "d3d12/RendererD3D12.h"

#include <atomic>



#include "entity/components/Mesh.h"
#include "d3d12/GraphicsDevice.h"
#include "d3d12/DeviceQueue.h"
#include "RenderInterface.h"



namespace RB::Graphics
{
	DWORD WINAPI RenderLoop(PVOID param);

	Renderer::Renderer()
		: m_RenderState(RenderThreadState::Idle)
		, m_RenderTaskType(RenderThreadTaskType::None)
	{
		m_GraphicsInterface = RenderInterface::Create(false);
		m_CopyInterface = RenderInterface::Create(true);

		InitializeConditionVariable(&m_KickCV);
		InitializeCriticalSection(&m_KickCS);

		InitializeConditionVariable(&m_SyncCV);
		InitializeCriticalSection(&m_SyncCS);

		DWORD id;
		m_RenderThread = CreateThread(NULL, 0, RenderLoop, (PVOID)this, 0, &id);
		RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, m_RenderThread != 0, "Failed to create render thread");
		SetThreadPriority(m_RenderThread, THREAD_PRIORITY_HIGHEST);
	}

	Renderer::~Renderer()
	{
		SendRenderThreadTask(RenderThreadTaskType::Shutdown);

		WaitForMultipleObjects(1, &m_RenderThread, TRUE, INFINITE);
		CloseHandle(m_RenderThread);
		DeleteCriticalSection(&m_KickCS);
		DeleteCriticalSection(&m_SyncCS);

		delete m_GraphicsInterface;
		delete m_CopyInterface;
	}

	void Renderer::SubmitFrameContext(const Entity::Scene* const scene)
	{
		// Retrieve all camera's with a valid output location and create ViewContexts for them
		// Also create the render entries? Or should those just be passed in?
	}

	void Renderer::StartResourceStreaming(const Entity::Scene* const scene /*, uint32_t min_time_to_upload_ms*/)
	{
		m_RenderTaskData = (void*) scene;
		SendRenderThreadTask(RenderThreadTaskType::ResourceStreaming);
	}

	void Renderer::SyncResourceStreaming()
	{
		// TODO Tell the render thread the main thread is waiting for the uploading to stop

		BlockUntilRenderThreadIdle();
	}

	void Renderer::StartRenderFrame()
	{
		SendRenderThreadTask(RenderThreadTaskType::FrameRender);
	}

	void Renderer::SyncRenderFrame()
	{
		BlockUntilRenderThreadIdle();
	}

	void Renderer::SendRenderThreadTask(RenderThreadTaskType task_type)
	{
		// Kick render thread
		EnterCriticalSection(&m_KickCS);
		m_RenderTaskType = task_type;
		m_RenderState = RenderThreadState::Waking;
		LeaveCriticalSection(&m_KickCS);
		WakeConditionVariable(&m_KickCV);
	}

	void Renderer::BlockUntilRenderThreadIdle()
	{
		// Sync with render thread
		EnterCriticalSection(&m_SyncCS);

		while (m_RenderState != RenderThreadState::Idle)
		{
			SleepConditionVariableCS(&m_SyncCV, &m_SyncCS, INFINITE);
		}

		LeaveCriticalSection(&m_SyncCS);
	}

	Renderer* Renderer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
			return new D3D12::RendererD3D12();
		default:
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Did not yet implement the Renderer for the set graphics API");
			break;
		}

		return nullptr;
	}

	DWORD WINAPI RenderLoop(PVOID param)
	{
		Renderer* r = (Renderer*)param;

		r->m_RenderState = Renderer::RenderThreadState::Idle;
		r->m_RenderTaskType = Renderer::RenderThreadTaskType::None;

		while (true)
		{
			// Wait until kick
			{
				EnterCriticalSection(&r->m_KickCS);

				while (r->m_RenderState == Renderer::RenderThreadState::Idle)
				{
					SleepConditionVariableCS(&r->m_KickCV, &r->m_KickCS, INFINITE);
				}

				LeaveCriticalSection(&r->m_KickCS);
			}

			if (r->m_RenderTaskType == Renderer::RenderThreadTaskType::Shutdown)
			{
				break;
			}

			r->m_RenderState = Renderer::RenderThreadState::Running;

			switch (r->m_RenderTaskType)
			{
			case Renderer::RenderThreadTaskType::FrameRender:
			{
				r->OnFrameStart();

				// Render



				// Preset

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

				r->OnFrameEnd();
			}
			break;

			case Renderer::RenderThreadTaskType::ResourceStreaming:
			{
				/*
					(Slowly) Upload needed resources for next frame, like new vertex data or textures.
					Constantly check if the main thread is waiting until this task is done, if so, finish uploading only the most crucial resources 
					and then stop (so textures are not completely crucial, as we can use the lower mips as fallback, or even white textures).
					Maybe an idea to give this task always a minimum amount of time to let it upload resources, because when maybe the main thread
					is not doing a lot, this task will not get a lot of time actually uploading stuff.

					Make sure to put a GPU wait after uploading the last resource of the task!
				*/

				const Entity::Scene* const scene = (Entity::Scene*)r->m_RenderTaskData;

				static List<const Entity::Mesh*> already_uploaded;

				List<const Entity::Mesh*> meshes = scene->GetComponentsWithTypeOf<Entity::Mesh>();

				for (const Entity::Mesh* mesh : meshes)
				{
					if (std::find(already_uploaded.begin(), already_uploaded.end(), mesh) != already_uploaded.end())
					{
						continue;
					}

					r->m_CopyInterface->UploadDataToResource(mesh->GetVertexBuffer(), mesh->GetVertexBuffer()->GetData(), mesh->GetVertexBuffer()->GetSize());
					already_uploaded.push_back(mesh);
				}

				Shared<RIExecutionGuard> guard = r->m_CopyInterface->ExecuteOnGpu();
				r->m_GraphicsInterface->GpuWaitOn(guard.get());
			}
			break;

			default:
				RB_LOG_WARN(LOGTAG_GRAPHICS, "Render task type not yet implemented");
				break;
			}

			// Notify that render thread is done with the frame
			{
				EnterCriticalSection(&r->m_SyncCS);
				r->m_RenderTaskType = Renderer::RenderThreadTaskType::None;
				r->m_RenderState = Renderer::RenderThreadState::Idle;
				LeaveCriticalSection(&r->m_SyncCS);
				WakeConditionVariable(&r->m_SyncCV);
			}
		}

		r->m_RenderState = Renderer::RenderThreadState::Terminated;

		return 0;
	}
}