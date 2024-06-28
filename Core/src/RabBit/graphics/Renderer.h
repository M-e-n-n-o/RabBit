#pragma once

#include "Window.h"
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
	class GpuGuard;
	class ViewContext;

	// Fully static class
	class Renderer : public Input::Events::EventListener
	{
	public:
		virtual ~Renderer();

		static void SetAPI(RenderAPI api) { s_Api = api; }
		inline static RenderAPI GetAPI() { return s_Api; }

		// Submits current frame relevant information of the scene to the renderer
		void SubmitFrame(const Entity::Scene* const scene);

		// Sync with the render thread (and optionally also wait until GPU is idle)
		void SyncRenderer(bool gpu_sync = false);

		uint64_t GetRenderFrameIndex();

		// Also syncs with the render thread and GPU
		void Shutdown();

		static Renderer* Create(bool enable_validation_layer);

	protected:
		Renderer(bool multi_threading_support);

		void Init();

		virtual void OnFrameStart() = 0;
		virtual void OnFrameEnd() = 0;

		virtual void SyncWithGpu() = 0;

	private:
		ViewContext* CreateViewContexts(const Entity::Scene* const scene, uint32_t& out_context_count);

		// Should only be called from the render thread!
		void OnEvent(Input::Events::Event& event) override;

		inline static RenderAPI s_Api = RenderAPI::None;

		bool							m_IsShutdown;
		WorkerThread*					m_RenderThread;
		JobTypeID						m_RenderJobType;
		JobTypeID						m_EventJobType;

		RenderInterface*				m_GraphicsInterface; // Used by the render passes
		RenderInterface*				m_CopyInterface;	 // Used for resource streaming 
		RenderPass*						m_StreamingPass;
		RenderPass**					m_RenderPasses;
		uint32_t						m_TotalPasses;

		uint64_t						m_RenderFrameIndex;
		CRITICAL_SECTION				m_RenderFrameIndexCS;

		bool							m_MultiThreadingSupport;

	public:
		struct BackBufferGuard
		{
			Shared<GpuGuard> guards[BACK_BUFFER_COUNT];
		};
	private:

		List<BackBufferGuard>			m_BackBufferAvailabilityGuards;

		const uint32_t					m_RenderThreadTimeoutMs = 100;
	};
}