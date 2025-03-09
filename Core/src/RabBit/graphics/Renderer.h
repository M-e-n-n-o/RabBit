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
    class GpuGuard;
    class ViewContext;
    class ResourceStreamer;
    class VertexBuffer;
    class RenderGraph;
    class RenderGraphContext;

    enum RenderGraphType
    {
        kRenderGraphType_Normal = 0,

        kRenderGraphType_Count
    };

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

        ResourceStreamer* GetStreamer() const { return m_ResourceStreamer; }

        uint64_t GetRenderFrameIndex();

        void Init();

        // Also syncs with the render thread and GPU
        void Shutdown();

        static Renderer* Create(bool enable_validation_layer);

    protected:
        Renderer(bool multi_threading_support);

        virtual void OnFrameStart() = 0;
        virtual void OnFrameEnd() = 0;

        virtual void SyncWithGpu() = 0;

    private:
        ViewContext* CreateViewContexts(const Entity::Scene* const scene, uint32_t& out_context_count);

        // Should only be called from the render thread!
        void OnEvent(Input::Events::Event& event) override;

        inline static RenderAPI s_Api = RenderAPI::None;

        bool						m_IsShutdown;
        WorkerThread*               m_RenderThread;
        JobTypeID					m_RenderJobType;

        RenderInterface*            m_GraphicsInterface; // Used by the render graphs
        RenderInterface*            m_CopyInterface;	 // Used for resource streaming 
        RenderGraph*                m_RenderGraphs[kRenderGraphType_Count];
        RenderGraphContext*         m_RenderGraphContext;

        ThreadedVariable<uint64_t>	m_RenderFrameIndex;
        ThreadedVariable<bool>		m_ForceSync;

        VertexBuffer*               m_BackBufferCopyVB;

        bool						m_MultiThreadingSupport;

        ResourceStreamer*           m_ResourceStreamer;

    public:
        struct BackBufferGuard
        {
            Shared<GpuGuard> guards[BACK_BUFFER_COUNT];
        };
    private:

        List<BackBufferGuard>		m_BackBufferAvailabilityGuards;

        const uint32_t				m_RenderThreadTimeoutMs = 100;
    };
}