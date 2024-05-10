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
			void*	 data;
			uint64_t dataSize;
		};

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

	private:
		void SendRenderThreadTask(RenderThreadTaskType task_type, const RenderTaskData& task_data);

		inline static RenderAPI s_Api = RenderAPI::None;

		extern friend DWORD WINAPI RenderLoop(PVOID param);
		extern friend DWORD WINAPI ResourceStreamLoop(PVOID param);

		HANDLE				m_RenderThread;
		ThreadState			m_RenderState;
		CONDITION_VARIABLE	m_RenderKickCV;
		CRITICAL_SECTION	m_RenderKickCS;
		CONDITION_VARIABLE	m_RenderSyncCV;
		CRITICAL_SECTION	m_RenderSyncCS;

		const uint32_t		m_RenderThreadTimeoutMs = 500;
		double				m_PerformanceFreqMs;
		uint64_t			m_RenderCounterStart;

		HANDLE				m_StreamingThread;
		ThreadState			m_StreamingState;
		CONDITION_VARIABLE	m_StreamingKickCV;
		CRITICAL_SECTION	m_StreamingKickCS;
		CONDITION_VARIABLE	m_StreamingSyncCV;
		CRITICAL_SECTION	m_StreamingSyncCS;
		void*				m_SharedRenderStreamingData;

		RenderTaskData		m_RenderTasks[RenderThreadTaskType::Count];

		RenderInterface*	m_CopyInterface;
		RenderInterface*	m_GraphicsInterface;
	};
}