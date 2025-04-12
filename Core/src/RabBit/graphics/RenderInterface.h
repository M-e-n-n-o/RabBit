#pragma once

#include "View.h"

namespace RB::Math
{
    struct Float4;
}

namespace RB::Graphics
{
    // GPU markers
#ifdef RB_ENABLE_LOGS
    #define RB_PROFILE_GPU_SCOPED(render_interface, name)				ProfileGpuScoped rb_profile_gpu_scoped(render_interface, 0, name);
    #define RB_PROFILE_GPU_SCOPED_COLOR(render_interface, name, color)	ProfileGpuScoped rb_profile_gpu_scoped(render_interface, color, name);
#else
    #define RB_PROFILE_GPU_SCOPED(render_interface, name)		
    #define RB_PROFILE_GPU_SCOPED_COLOR(render_interface, name, color) 
#endif

    #define INTERMEDIATE_EXECUTE_THRESHOLD 350

    class RenderResource;
    class RenderTargetBundle;
    enum class ResourceState;

    enum class BlendMode
    {
        None,
        SrcAlphaLerp
    };

    enum class CullMode
    {
        None,
        Front,
        Back
    };

    enum class DepthMode
    {
        PassAll,
        PassFurther,
        PassCloser,
    };

    class GpuGuard
    {
    public:
        virtual ~GpuGuard() = default;

        virtual bool IsFinishedRendering() = 0;
        virtual void WaitUntilFinishedRendering() = 0;

    protected:
        GpuGuard() = default;
    };

    class RenderInterface
    {
    public:
        virtual ~RenderInterface() = default;

        virtual void InvalidateState(bool executed_external_code) = 0;

        Shared<GpuGuard> ExecuteOnGpu();

        virtual void GpuWaitOn(GpuGuard* guard) = 0;

        // You should normally not have to manually transition resources, this will be done automatically
        virtual void TransitionResource(RenderResource* resource, ResourceState state) = 0;
        virtual void FlushResourceBarriers() = 0;
        // Flushes all possible pending things, also resource barriers
        virtual void FlushAllPending() = 0;

        virtual void SetVertexShader(uint32_t shader_index) = 0;
        virtual void SetPixelShader(uint32_t shader_index) = 0;

        virtual void SetIndexBuffer(RenderResource* index_resource) = 0;
        virtual void SetVertexBuffer(RenderResource* vertex_resource, uint32_t slot = 0) = 0;
        virtual void SetVertexBuffers(RenderResource** vertex_resources, uint32_t resource_count, uint32_t start_slot = 0) = 0;

        virtual void SetRenderTarget(RenderTargetBundle* bundle) = 0;
        virtual void SetRenderTarget(RenderResource* color_target) = 0;

        virtual void SetConstantShaderData(uint32_t slot, void* data, uint32_t data_size) = 0;

        virtual void SetShaderResourceInput(RenderResource* resource, uint32_t slot) = 0;
        virtual void ClearShaderResourceInput(uint32_t slot) = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetViewports(const Viewport* viewports, uint32_t total_viewports) = 0;

        virtual void SetBlendMode(const BlendMode& mode) = 0;
        virtual void SetCullMode(const CullMode& mode) = 0;
        virtual void SetDepthMode(const DepthMode& mode, bool write_depth, bool reversed_depth) = 0;

        virtual void Clear(RenderResource* resource); // Clears to black
        virtual void Clear(RenderResource* resource, const Math::Float4& color) = 0;
        virtual void ClearDepth(RenderResource* resource, bool reversed_depth);

        virtual void UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size) = 0;
        virtual void CopyResource(RenderResource* src, RenderResource* dest) = 0;

        void Draw();
        void Dispatch();

        virtual void ProfileMarkerBegin(uint64_t color, const char* name) = 0;
        virtual void ProfileMarkerEnd() = 0;

        static RenderInterface* Create(bool allow_only_copy_operations);

    protected:
        RenderInterface() = default;

        bool NeedsIntermediateExecute();

        virtual Shared<GpuGuard> ExecuteInternal() = 0;
        virtual void DrawInternal() = 0;
        virtual void DispatchInternal() = 0;

        uint32_t m_TotalDraws = 0;
    };

#ifdef RB_ENABLE_LOGS
    class ProfileGpuScoped
    {
    public:
        ProfileGpuScoped(RenderInterface* i, uint64_t color, const char* name)
        {
            m_Interface = i;
            m_Interface->ProfileMarkerBegin(color, name);
        }

        ~ProfileGpuScoped()
        {
            m_Interface->ProfileMarkerEnd();
        }

    private:
        RenderInterface* m_Interface;
    };
#endif
}