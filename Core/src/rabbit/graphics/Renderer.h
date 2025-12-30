#pragma once

#include "Window.h"
#include "events/Event.h"
#include "utils/Threading.h"
#include "app/FrameAllocator.h"
#include "RenderGraph.h"

namespace RB::Entity
{
    class Scene;
}

namespace RB::Graphics
{
    enum class RenderAPI
    {
        None,
        D3D12,
        Vulkan
    };

    class RenderInterface;
    class GpuGuard;
    class ViewContext;
    class ResourceStreamer;
    class VertexBuffer;

    enum RenderGraphType
    {
        kRenderGraphType_Normal = 0,

        // TODO: When implementing upscaling, we still want to render the UI at full res.
        //       Probably good to make a separate graph for the UI rendering and just use
        //       the output of regular rendering as input to the UI graph.
        //kRenderGraphType_Post = 1,

        kRenderGraphType_Count
    };

    class Renderer : public Events::EventListener
    {
    public:
        virtual ~Renderer();

        static void SetAPI(RenderAPI api);
        inline static RenderAPI GetAPI() { return s_Api; }

        // Submits current frame relevant information of the scene to the renderer
        void SubmitFrame(const Entity::Scene* const scene);

        // Sync with the render thread (and optionally also wait until GPU is idle)
        // Should only be called from the Main or Render thread!
        void SyncRenderer(bool gpu_sync = false);

        void SetRenderGraph(RenderGraphType graph_type, const RenderGraphBuilder& graph);

        ResourceStreamer* GetStreamer() const { return m_ResourceStreamer; }
        
        FrameAllocator* GetAllocator() const { return m_RenderAllocator; }

        uint64_t GetRenderFrameIndex();

        void Init();

        // Also syncs with the render thread and GPU
        void Shutdown();

        static Renderer* Create(bool enable_validation_layer, bool load_pix_lib);

    protected:
        Renderer(bool multi_threading_support);

        virtual void OnFrameStart() = 0;
        virtual void OnFrameEnd() = 0;

        virtual void SyncWithGpu() = 0;

    private:
        ViewContext* CreateViewContexts(const Entity::Scene* const scene, uint32_t& out_context_count);
        void UpdateRenderGraphSizes(const ViewContext* view_contexts, uint32_t context_count);

        // Should only be called from the render thread!
        bool OnEvent(Events::Event& event) override;

        inline static RenderAPI s_Api = RenderAPI::None;

        bool                        m_IsShutdown;
        WorkerThread*               m_RenderThread;
        JobTypeID                   m_RenderJobType;

        RenderInterface*            m_GraphicsInterface; // Used by the render graphs
        RenderInterface*            m_CopyInterface;	 // Used for resource streaming 
        RenderGraph*                m_RenderGraphs[kRenderGraphType_Count];
        RenderGraphContext*         m_RenderGraphContext;
        uint32_t                    m_CurrentValidRenderGraphSizes;
        uint32_t*                   m_RenderGraphSizeIDs;

        ThreadedVariable<uint64_t>  m_RenderFrameIndex;
        ThreadedVariable<uint32_t>  m_ForceSync;

        Shared<VertexBuffer>        m_BackBufferCopyVB;

        bool                        m_MultiThreadingSupport;

        ResourceStreamer*           m_ResourceStreamer;

        FrameAllocator*             m_RenderAllocator;

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