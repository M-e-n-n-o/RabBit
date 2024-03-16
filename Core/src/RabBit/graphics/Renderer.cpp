#include "RabBitCommon.h"
#include "Renderer.h"
#include "RenderInterface.h"
#include "d3d12/RendererD3D12.h"

#include <atomic>

namespace RB::Graphics
{
	DWORD WINAPI RenderLoop(PVOID param);

	Renderer::Renderer()
		: m_RenderInterface(nullptr)
		, m_RenderState(RenderThreadState::Idle)
	{
		InitializeConditionVariable(&m_KickCV);
		InitializeCriticalSection(&m_KickCS);

		InitializeConditionVariable(&m_SyncCV);
		InitializeCriticalSection(&m_SyncCS);

		DWORD id;
		m_RenderThread = CreateThread(NULL, 0, RenderLoop, (PVOID)this, 0, &id);
		SetThreadPriority(m_RenderThread, THREAD_PRIORITY_HIGHEST);
	}

	Renderer::~Renderer()
	{
		EnterCriticalSection(&m_KickCS);
		m_RenderState = RenderThreadState::Shutdown;
		LeaveCriticalSection(&m_KickCS);
		WakeConditionVariable(&m_KickCV);

		WaitForMultipleObjects(1, &m_RenderThread, TRUE, INFINITE);
		CloseHandle(m_RenderThread);
		DeleteCriticalSection(&m_KickCS);
		DeleteCriticalSection(&m_SyncCS);

		SAFE_DELETE(m_RenderInterface);
	}
	
	RenderInterface* Renderer::GetRenderInterface()
	{
		if (m_RenderInterface == nullptr)
		{
			m_RenderInterface = RenderInterface::Create();
		}

		return m_RenderInterface;
	}

	void Renderer::StartRenderFrame()
	{
		// Kick render thread
		EnterCriticalSection(&m_KickCS);
		m_RenderState = RenderThreadState::Waking;
		LeaveCriticalSection(&m_KickCS);
		WakeConditionVariable(&m_KickCV);
	}

	void Renderer::FinishRenderFrame()
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
	}

	DWORD WINAPI RenderLoop(PVOID param)
	{
		Renderer* r = (Renderer*)param;

		r->m_RenderState = RenderThreadState::Idle;

		while (true)
		{
			// Wait until kick
			{
				EnterCriticalSection(&r->m_KickCS);

				while (r->m_RenderState == RenderThreadState::Idle)
				{
					SleepConditionVariableCS(&r->m_KickCV, &r->m_KickCS, INFINITE);
				}

				LeaveCriticalSection(&r->m_KickCS);
			}

			if (r->m_RenderState == RenderThreadState::Shutdown)
			{
				break;
			}

			r->m_RenderState = RenderThreadState::Running;

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
			*/

			// Notify that render thread is done with the frame
			{
				EnterCriticalSection(&r->m_SyncCS);
				r->m_RenderState = RenderThreadState::Idle;
				LeaveCriticalSection(&r->m_SyncCS);
				WakeConditionVariable(&r->m_SyncCV);
			}
		}

		r->m_RenderState = RenderThreadState::Terminated;

		return 0;
	}
}