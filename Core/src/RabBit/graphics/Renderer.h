#pragma once

#include "input/events/Event.h"

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
		enum class ThreadState
		{
			Idle,
			Running,
			Terminated
		};

		enum RenderThreadTaskType : uint8_t
		{
			Stop,
			RenderFrame,
			HandleEvents,

			Count
		};

		struct RenderTask
		{
			bool hasTask;
			void* data;
			uint64_t dataSize;
		};

		struct SharedRenderThreadContext
		{
			RenderPass**		renderPasses;
			uint32_t			totalPasses;

			ThreadState			renderState;
			CONDITION_VARIABLE	renderKickCV;
			CRITICAL_SECTION	renderKickCS;
			CONDITION_VARIABLE	renderSyncCV;
			CRITICAL_SECTION	renderSyncCS;

			const uint32_t		renderThreadTimeoutMs = 500;
			double				performanceFreqMs;
			uint64_t			renderCounterStart;

			HANDLE				streamingThread;
			ThreadState			streamingState;
			CONDITION_VARIABLE	streamingKickCV;
			CRITICAL_SECTION	streamingKickCS;
			CONDITION_VARIABLE	streamingSyncCV;
			CRITICAL_SECTION	streamingSyncCS;

			RenderTask			renderTasks[RenderThreadTaskType::Count];
			void*				sharedRenderStreamingData;

			RenderInterface*	copyInterface;
			RenderInterface*	graphicsInterface;

			uint64_t			renderFrameIndex;
			CRITICAL_SECTION	renderFrameIndexCS;

			std::function<void()> OnRenderFrameStart;
			std::function<void()> OnRenderFrameEnd;
			std::function<void()> ProcessEvents;
			std::function<void()> SyncWithGpu;
		};

	private:
		// Should only be called from the render thread!
		void OnEvent(Input::Events::Event& event) override;

		void SendRenderThreadTask(RenderThreadTaskType task_type, const RenderTask& task);

		RenderTask GetDummyTask();

		inline static RenderAPI s_Api = RenderAPI::None;

		HANDLE						m_RenderThread;
		SharedRenderThreadContext*	m_SharedContext;
		bool						m_IsShutdown;
	};
}