#include "Renderer.h"
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
	DWORD WINAPI ResourceStreamLoop(PVOID param);

	Renderer::Renderer()
		: m_RenderState(ThreadState::Idle)
	{
		for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
		{
			m_RenderTasks[task_type]			= {};
			m_RenderTasks[task_type].data		= nullptr;
			m_RenderTasks[task_type].dataSize	= 0;
		}

		LARGE_INTEGER li;
		if (!QueryPerformanceFrequency(&li))
		{
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Could not retrieve value from QueryPerformanceFrequency");
		}

		m_PerformanceFreqMs = double(li.QuadPart) / 1000.0;
	}

	void Renderer::Init()
	{
		m_GraphicsInterface = RenderInterface::Create(false);
		m_CopyInterface = RenderInterface::Create(true);

		InitializeConditionVariable(&m_RenderKickCV);
		InitializeCriticalSection(&m_RenderKickCS);
		InitializeConditionVariable(&m_RenderSyncCV);
		InitializeCriticalSection(&m_RenderSyncCS);

		DWORD id;
		m_RenderThread = CreateThread(NULL, 0, RenderLoop, (PVOID)this, 0, &id);
		RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, m_RenderThread != 0, "Failed to create render thread");
		SetThreadDescription(m_RenderThread, L"Render Thread");
		SetThreadPriority(m_RenderThread, THREAD_PRIORITY_HIGHEST);
	}

	Renderer::~Renderer()
	{
		RenderTaskData dummy_data = {};
		dummy_data.data		= new uint8_t(0);
		dummy_data.dataSize = sizeof(uint8_t);

		SendRenderThreadTask(RenderThreadTaskType::Shutdown, dummy_data);

		WaitForMultipleObjects(1, &m_RenderThread, TRUE, INFINITE);
		CloseHandle(m_RenderThread);
		DeleteCriticalSection(&m_RenderKickCS);
		DeleteCriticalSection(&m_RenderSyncCS);

		delete m_GraphicsInterface;
		delete m_CopyInterface;
	}

	void Renderer::SubmitFrameContext(const Entity::Scene* const scene)
	{
		// Retrieve all camera's with a valid output location and create ViewContexts for them
		// Also create the render entries? Or should those just be passed in?

		void* submitted_data; // <- Make sure to allocate this on the heap (using new, not malloc!)

		RenderTaskData data;
		data.data = submitted_data;
		data.dataSize = sizeof(submitted_data);

		// Sync with the renderer if it is stalling
		{
			EnterCriticalSection(&m_RenderKickCS);
			ThreadState render_state  = m_RenderState;
			uint64_t	counter_start = m_RenderCounterStart;
			LeaveCriticalSection(&m_RenderKickCS);

			if (render_state != ThreadState::Idle)
			{
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);

				if ((double(li.QuadPart - counter_start) / m_PerformanceFreqMs) > m_RenderThreadTimeoutMs)
				{
					SyncRenderer(false);
				}
			}
		}

		SendRenderThreadTask(RenderThreadTaskType::RenderFrame, data);
	}

	void Renderer::SyncRenderer(bool gpu_sync)
	{
		// Sync with render thread
		EnterCriticalSection(&m_RenderSyncCS);

		while (m_RenderState != ThreadState::Idle)
		{
			SleepConditionVariableCS(&m_RenderSyncCV, &m_RenderSyncCS, INFINITE);
		}

		LeaveCriticalSection(&m_RenderSyncCS);

		if (gpu_sync)
		{
			SyncWithGpu();
		}
	}

	void Renderer::SendRenderThreadTask(RenderThreadTaskType task_type, const RenderTaskData& task_data)
	{
		// Kick render thread
		EnterCriticalSection(&m_RenderKickCS);
		m_RenderTasks[task_type] = task_data;
		LeaveCriticalSection(&m_RenderKickCS);
		WakeConditionVariable(&m_RenderKickCV);
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
		Renderer* r = (Renderer*)param;

		// Spawn the streaming thread
		{
			InitializeConditionVariable(&r->m_StreamingKickCV);
			InitializeCriticalSection(&r->m_StreamingKickCS);
			InitializeConditionVariable(&r->m_StreamingSyncCV);
			InitializeCriticalSection(&r->m_StreamingSyncCS);

			DWORD id;
			r->m_StreamingThread = CreateThread(NULL, 0, ResourceStreamLoop, (PVOID)r, 0, &id);
			RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, r->m_StreamingThread != 0, "Failed to create resource streaming thread");
			SetThreadDescription(r->m_StreamingThread, L"Resource Streaming Thread");
			SetThreadPriority(r->m_StreamingThread, THREAD_PRIORITY_HIGHEST);
		}

		bool stop = false;
		while (stop)
		{
			void* render_data[Renderer::RenderThreadTaskType::Count];

			{
				EnterCriticalSection(&r->m_RenderKickCS);

				// Reset timer
				r->m_RenderCounterStart = 0;

				// Wait until a new task is available
				while (true)
				{
					bool has_new_task = false;
					for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
					{
						if (r->m_RenderTasks[task_type].dataSize > 0)
						{
							has_new_task = true;
							break;
						}
					}

					if (has_new_task)
					{
						break;
					}

					// Notify the sync that we are starting to idle
					{
						EnterCriticalSection(&r->m_RenderSyncCS);
						r->m_RenderState = Renderer::ThreadState::Idle;
						LeaveCriticalSection(&r->m_RenderSyncCS);
						WakeConditionVariable(&r->m_RenderSyncCV);
					}

					SleepConditionVariableCS(&r->m_RenderKickCV, &r->m_RenderKickCS, INFINITE);
				}

				r->m_RenderState = Renderer::ThreadState::Running;

				// Copy the task data
				for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
				{
					if (r->m_RenderTasks[task_type].dataSize <= 0)
					{
						continue;
					}

					render_data[task_type] = ALLOC_HEAP(r->m_RenderTasks[task_type].dataSize);

					memcpy(render_data[task_type], r->m_RenderTasks[task_type].data, r->m_RenderTasks[task_type].dataSize);

					SAFE_DELETE(r->m_RenderTasks[task_type].data);
				}

				// Start timer
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				r->m_RenderCounterStart = li.QuadPart;

				LeaveCriticalSection(&r->m_RenderKickCS);
			}

			// Execute the tasks
			for (uint8_t task_type = 0; task_type < Renderer::RenderThreadTaskType::Count; ++task_type)
			{
				if (render_data[task_type] == nullptr)
				{
					continue;
				}

				switch (task_type)
				{
				case Renderer::RenderThreadTaskType::Shutdown:
				{
					stop = true;

					// Also stop streaming thread
					{
						EnterCriticalSection(&r->m_StreamingKickCS);
						r->m_StreamingState = Renderer::ThreadState::Running;
						r->m_SharedRenderStreamingData = nullptr;
						LeaveCriticalSection(&r->m_StreamingKickCS);
						WakeConditionVariable(&r->m_StreamingKickCV);
					}
				}
				break;

				case Renderer::RenderThreadTaskType::RenderFrame:
				{
					r->OnFrameStart();

					// Kick off streaming thread
					{
						EnterCriticalSection(&r->m_StreamingKickCS);
						r->m_StreamingState = Renderer::ThreadState::Running;
						r->m_SharedRenderStreamingData = render_data[task_type];
						LeaveCriticalSection(&r->m_StreamingKickCS);
						WakeConditionVariable(&r->m_StreamingKickCV);
					}

					// Render

					// Present

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
						EnterCriticalSection(&r->m_StreamingSyncCS);

						while (r->m_StreamingState != Renderer::ThreadState::Idle)
						{
							SleepConditionVariableCS(&r->m_StreamingSyncCV, &r->m_StreamingSyncCS, INFINITE);
						}

						r->m_SharedRenderStreamingData = nullptr;

						LeaveCriticalSection(&r->m_StreamingSyncCS);
					}

					r->OnFrameEnd();
				}

				default:
					RB_LOG_WARN(LOGTAG_GRAPHICS, "Render task type not yet implemented");
					break;
				}

				if (stop)
				{
					break;
				}

				SAFE_FREE( render_data[task_type]);
			}
		}

		WaitForMultipleObjects(1, &r->m_StreamingThread, TRUE, INFINITE);
		CloseHandle(r->m_StreamingThread);
		DeleteCriticalSection(&r->m_StreamingKickCS);
		DeleteCriticalSection(&r->m_StreamingSyncCS);

		r->m_RenderState = Renderer::ThreadState::Terminated;

		return 0;
	}

	DWORD WINAPI ResourceStreamLoop(PVOID param)
	{
		Renderer* r = (Renderer*)param;

		r->m_StreamingState = Renderer::ThreadState::Idle;

		while (true)
		{
			// Wait until kick from render thread
			{
				EnterCriticalSection(&r->m_StreamingKickCS);

				while (r->m_StreamingState == Renderer::ThreadState::Idle)
				{
					SleepConditionVariableCS(&r->m_StreamingKickCV, &r->m_StreamingKickCS, INFINITE);
				}

				LeaveCriticalSection(&r->m_StreamingKickCS);
			}

			if (r->m_SharedRenderStreamingData == nullptr)
			{
				// Terminate
				break;
			}

			// Do resource management/streaming
			{
				/*
				 
					!!!!! RESOURCE STREAMING THREAD SHOULD ONLY SCHEDULE THINGS ON THE COPY INTERFACE, NO GRAPHICS STUFF !!!!!!
				
				*/


				//r->m_SharedRenderStreamingData


				//case Renderer::RenderThreadTaskType::ResourceManagement:
				//{
				//	/*
				//		(Slowly) Upload needed resources for next frame, like new vertex data or textures.
				//		Constantly check if the main thread is waiting until this task is done, if so, finish uploading only the most crucial resources 
				//		and then stop (so textures are not completely crucial, as we can use the lower mips as fallback, or even white textures).
				//		Maybe an idea to give this task always a minimum amount of time to let it upload resources, because when maybe the main thread
				//		is not doing a lot, this task will not get a lot of time actually uploading stuff.

				//		Make sure to put a GPU wait after uploading the last resource of the task!
				//	*/

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
				EnterCriticalSection(&r->m_StreamingSyncCS);
				r->m_StreamingState = Renderer::ThreadState::Idle;
				LeaveCriticalSection(&r->m_StreamingSyncCS);
				WakeConditionVariable(&r->m_StreamingSyncCV);
			}
		}

		r->m_StreamingState = Renderer::ThreadState::Terminated;

		return 0;
	}
}