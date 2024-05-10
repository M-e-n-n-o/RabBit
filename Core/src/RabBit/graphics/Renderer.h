#pragma once

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

	// Fully static class
	class Renderer
	{
	public:
		virtual ~Renderer();

		static void SetAPI(RenderAPI api) { s_Api = api; }
		inline static RenderAPI GetAPI() { return s_Api; }

		// Submits current frame relevant information of the scene to the renderer
		void SubmitFrameContext(const Entity::Scene* const scene);

		// Sync with the render thread (and optionally also wait until GPU is idle)
		void SyncRenderer(bool gpu_sync = false);

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
			Shutdown,
			// TODO Also make a task for a resource resize (if the backbuffer needs to be resized)
			RenderFrame,

			Count
		};

		struct RenderTaskData
		{
			void* data;
			uint64_t dataSize;
		};

		struct SharedRenderThreadContext
		{
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

			RenderTaskData		renderTasks[RenderThreadTaskType::Count];
			void*				sharedRenderStreamingData;

			RenderInterface*	copyInterface;
			RenderInterface*	graphicsInterface;

			std::function<void()> OnRenderFrameStart;
			std::function<void()> OnRenderFrameEnd;
		};

	private:
		void SendRenderThreadTask(RenderThreadTaskType task_type, const RenderTaskData& task_data);

		inline static RenderAPI s_Api = RenderAPI::None;

		HANDLE						m_RenderThread;
		SharedRenderThreadContext*	m_SharedContext;

	};
}