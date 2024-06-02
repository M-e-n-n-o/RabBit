#pragma once

#include "input/events/Event.h"
#include "utils/Threading.h"

namespace RB::Entity
{
	class Scene;
}

namespace RB::Graphics
{
	enum class RenderAPI
	{
		None,
		D3D12
	};

	class RenderInterface;
	class RenderPass;
	struct RenderPassContext;

	// Fully static class
	class Renderer : public Input::Events::EventListener
	{
	public:
		virtual ~Renderer();

		static void SetAPI(RenderAPI api) { s_Api = api; }
		inline static RenderAPI GetAPI() { return s_Api; }

		// Submits current frame relevant information of the scene to the renderer
		void SubmitFrameContext(const Entity::Scene* const scene);

		// Sync with the render thread (and optionally also wait until GPU is idle)
		void SyncRenderer(bool gpu_sync = false);

		uint64_t GetRenderFrameIndex() const;

		// Also syncs with the render thread and GPU
		void Shutdown();

		static Renderer* Create(bool enable_validation_layer);

	protected:
		Renderer();

		void Init();

		virtual void OnFrameStart() = 0;
		virtual void OnFrameEnd() = 0;

		virtual void SyncWithGpu() = 0;

	public:
		//enum class ThreadState
		//{
		//	Idle,
		//	Running,
		//	Terminated
		//};

		//enum RenderThreadTaskType : uint8_t
		//{
		//	Stop,
		//	RenderFrame,
		//	HandleEvents,

		//	Count
		//};

		//struct RenderTask
		//{
		//	bool hasTask;
		//	void* data;
		//	uint64_t dataSize;
		//};

		struct RenderContext
		{
			RenderPass**			renderPasses;
			RenderPassContext*		renderPassContexts;
			uint32_t				totalPasses;

			//ThreadState			renderState;
			//CONDITION_VARIABLE	renderKickCV;
			//CRITICAL_SECTION	renderKickCS;
			//CONDITION_VARIABLE	renderSyncCV;
			//CRITICAL_SECTION	renderSyncCS;

			//const uint32_t		renderThreadTimeoutMs = 500;
			//double				performanceFreqMs;
			//uint64_t			renderCounterStart;

			//HANDLE				streamingThread;
			//ThreadState			streamingState;
			//CONDITION_VARIABLE	streamingKickCV;
			//CRITICAL_SECTION	streamingKickCS;
			//CONDITION_VARIABLE	streamingSyncCV;
			//CRITICAL_SECTION	streamingSyncCS;

			//RenderTask			renderTasks[RenderThreadTaskType::Count];
			//void*				sharedRenderStreamingData;

			RenderInterface*		copyInterface;
			RenderInterface*		graphicsInterface;

			uint64_t*				renderFrameIndex;
			CRITICAL_SECTION*		renderFrameIndexCS;

			std::function<void()>	OnRenderFrameStart;
			std::function<void()>	OnRenderFrameEnd;
			std::function<void()>	SyncWithGpu;

			~RenderContext()
			{
				SAFE_FREE(renderPassContexts);
			}
		};

		struct EventContext
		{
			std::function<void()> ProcessEvents;
		};

	private:
		// Should only be called from the render thread!
		void OnEvent(Input::Events::Event& event) override;

		//void SendRenderThreadTask(RenderThreadTaskType task_type, const RenderTask& task);

		//RenderTask GetDummyTask();

		inline static RenderAPI s_Api = RenderAPI::None;

		bool				m_IsShutdown;
		WorkerThread*		m_RenderThread;
		JobTypeID			m_RenderJobType;
		JobTypeID			m_EventJobType;

		RenderInterface*	m_GraphicsInterface;
		RenderInterface*	m_CopyInterface;
		RenderPass**		m_RenderPasses;
		uint32_t			m_TotalPasses;

		uint64_t			m_RenderFrameIndex;
		CRITICAL_SECTION	m_RenderFrameIndexCS;

		const uint32_t		m_RenderThreadTimeoutMs = 500;


		//HANDLE						m_RenderThread;
		//SharedRenderThreadContext*	m_SharedContext;
	};
}