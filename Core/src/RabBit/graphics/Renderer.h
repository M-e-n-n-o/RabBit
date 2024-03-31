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
		enum class RenderThreadState
		{
			Idle,
			Running,
			Waking,
			Terminated
		};

		enum class RenderThreadTaskType
		{
			None,
			Shutdown,
			FrameRender,
			ResourceManagement

			// TODO Also make a task for a resource resize (if the backbuffer needs to be resized)
		};

		virtual ~Renderer();

		static void SetAPI(RenderAPI api) { s_Api = api; }
		inline static RenderAPI GetAPI() { return s_Api; }

		// Kick off frame render (is 1 frame behind game update)
		void StartRenderFrame();
		// Sync until render thread is finished with rendering frame (blocking call)
		void SyncRenderFrame();

		// Submits current frame relevant information of the scene to the renderer
		void SubmitFrameContext(const Entity::Scene* const scene);

		// Kick off resource uploading & texture streaming of the current' frame resources
		void StartResourceManagement(const Entity::Scene* const scene);
		// Tell the render thread we are waiting until the resource uploading is completed and wait until completed (blocking call)
		void SyncResourceManagement();

		static Renderer* Create();

	protected:
		Renderer();

		virtual void OnFrameStart() = 0;
		virtual void OnFrameEnd() = 0;

	private:
		void SendRenderThreadTask(RenderThreadTaskType task_type);
		void BlockUntilRenderThreadIdle();

		inline static RenderAPI s_Api = RenderAPI::None;

		extern friend DWORD WINAPI RenderLoop(PVOID param);

		HANDLE				m_RenderThread;
		CONDITION_VARIABLE	m_KickCV;
		CRITICAL_SECTION	m_KickCS;
		CONDITION_VARIABLE	m_SyncCV;
		CRITICAL_SECTION	m_SyncCS;

		RenderThreadState	 m_RenderState;
		RenderThreadTaskType m_RenderTaskType;
		void*				 m_RenderTaskData;

		RenderInterface* m_CopyInterface;
		RenderInterface* m_GraphicsInterface;
	};
}