#pragma once

#include "Window.h"
#include "RenderGraph.h"
#include "events/Event.h"
#include "utils/Threading.h"
#include "app/FrameAllocator.h"

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
    class ShaderSystem;
    class VertexBuffer;

    // Defines the different render graph types and the order in which they are rendered
    enum RenderGraphType
    {
        // TODO: 
        //  - When implementing upscaling, we still want to render the UI at full res.
        //       Probably good to make a separate graph for the UI rendering and just use
        //       the output of regular rendering as input to the UI graph.
        //  - Need to add support for custom rendertargets as an input to a renderpass.

        kRenderGraphType_Normal = 0,
        kRenderGraphType_Post,
        kRenderGraphType_Count
    };

    class Renderer : public Events::EventListener
    {
    public:
        virtual ~Renderer();

        inline static RenderAPI GetAPI() { return s_Api; }

        // Submits current frame relevant information of the scene to the renderer
        void SubmitFrame(const Entity::Scene* const scene);

        // Sync with the render thread (and optionally also wait until GPU is idle)
        // Should only be called from the Main or Render thread!
        void SyncRenderer(bool gpu_sync = false);

        void SetRenderGraphs(const UnorderedMap<RenderGraphType, RenderGraphBuilder>& graphs);

        ShaderSystem*     GetShaderSystem() const { return m_ShaderSystem; }
        FrameAllocator*   GetAllocator() const { return m_RenderAllocator; }

        uint64_t GetRenderFrameIndex();
        float GetLastRenderTime(); // In ms
        float GetLastPresentInverval();  // In ms

        void Init();

        // Also syncs with the render thread and GPU
        void Shutdown();

        static Renderer* Create(RenderAPI api, bool enable_validation_layer, bool load_pix_lib);

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

        RenderInterface*            m_GraphicsInterface;
        RenderGraph*                m_RenderGraphs[kRenderGraphType_Count];
        RenderGraphContext*         m_RenderGraphContext;
        uint32_t                    m_CurrentValidRenderGraphSizes;
        uint32_t*                   m_RenderGraphSizeIDs;

        ThreadedVariable<uint64_t>  m_RenderFrameIndex;
        ThreadedVariable<uint32_t>  m_ForceSync;

        Timer                       m_PresentTimer;
        double                      m_PresentTimeInverval;
        Mutex                       m_PresentTimerMutex;

        Shared<VertexBuffer>        m_BackBufferCopyVB;

        bool                        m_MultiThreadingSupport;

        ShaderSystem*               m_ShaderSystem;

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