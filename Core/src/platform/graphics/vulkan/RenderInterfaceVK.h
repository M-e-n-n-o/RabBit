#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "graphics/RenderInterface.h"
#include "graphics/RenderResource.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class DeviceQueue;

    class GpuGuardVK : public GpuGuard
    {
    public:
        GpuGuardVK(DeviceQueue* queue, uint64_t submission_value);

        bool IsFinishedRendering() override;
        void WaitUntilFinishedRendering() override;

    private:
        DeviceQueue* m_Queue;
        uint64_t     m_SubmissionValue;

        friend class RenderInterfaceVK;
    };

    class RenderInterfaceVK : public RenderInterface
    {
    public:
        RenderInterfaceVK(bool allow_only_copy_operations);
        ~RenderInterfaceVK();

        void InvalidateState(bool rebind_descriptor_heap) override {}

        Shared<GpuGuard> ExecuteInternal() override;
        void GpuWaitOn(GpuGuard* guard) override;

        void TransitionResource(RenderResource* resource, ResourceState state) override {}
        void FlushResourceBarriers() override {}
        void FlushAllPending() override {}

        void PushRenderTarget(RenderResource* color_target, uint32_t index = 0) override {}
        void PopRenderTarget(uint32_t index = 0) override {}
        void SetDepthStencil(RenderResource* ds_target) override {}
        void ClearRenderTargets() override {}

        void SetShaderResourceInput(RenderResource* resource, uint32_t slot) override {}
        void SetRandomReadWriteInput(RenderResource* resource, uint32_t slot) override {}
        void ClearShaderResourceInput(uint32_t slot) override {}
        void ClearRandomReadWriteInput(uint32_t slot) override {}

        void SetConstantShaderData(uint32_t slot, void* data, uint32_t data_size) override {}

        void SetVertexShader(uint32_t shader_index) override {}
        void SetPixelShader(uint32_t shader_index) override {}
        void SetComputeShader(uint32_t shader_index) override {}

        void Clear(RenderResource* resource, const Math::Float4& color) override {}

        void SetViewport(const Viewport& viewport) override {}
        void SetViewports(const Viewport* viewports, uint32_t total_viewports) override {}

        void SetBlendMode(const BlendMode& mode) override {}
        void SetCullMode(const CullMode& mode) override {}
        void SetDepthMode(const DepthMode& mode, bool write_depth, bool reversed_depth) override {}

        void SetIndexBuffer(RenderResource* index_resource) override {}
        void SetVertexBuffer(RenderResource* vertex_resource, uint32_t slot) override {}
        void SetVertexBuffers(RenderResource** vertex_resources, uint32_t resource_count, uint32_t start_slot) override {}

        void CopyResource(RenderResource* src, RenderResource* dest) override {}

        void UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size) override {}

        void DrawInternal() override {}
        void DispatchInternal(uint32_t thread_groups_x, uint32_t thread_groups_y, uint32_t thread_groups_z) override {}

        void ProfileMarkerBegin(uint64_t color, const char* name) override {}
        void ProfileMarkerEnd() override {}

    private:
        bool            m_CopyOperationsOnly;
        DeviceQueue*    m_Queue;
        VkCommandBuffer m_CommandBuffer;
    };
}
#endif