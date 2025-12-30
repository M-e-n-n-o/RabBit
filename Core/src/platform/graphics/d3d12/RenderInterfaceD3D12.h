#if RB_GRAPHICS_API_D3D12

#pragma once

#include "graphics/RenderInterface.h"
#include "graphics/RenderResource.h"
#include "graphics/shaders/shared/Common.h"
#include "resource/Descriptor.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    class DeviceQueue;
    class GpuResource;

    class GpuGuardD3D12 : public GpuGuard
    {
    public:
        GpuGuardD3D12(uint64_t fence_value, DeviceQueue* queue);

        bool IsFinishedRendering() override;
        void WaitUntilFinishedRendering() override;

    private:
        uint64_t				m_FenceValue;
        DeviceQueue*            m_Queue;

        friend class RenderInterfaceD3D12;
    };

    class RenderInterfaceD3D12 : public RenderInterface
    {
    public:
        RenderInterfaceD3D12(bool allow_only_copy_operations);
        ~RenderInterfaceD3D12();

        void InvalidateState(bool rebind_descriptor_heap) override;

        // This method executes the command list and sets a new internal valid command list
        // Returns the execute ID (on which can be waited)
        Shared<GpuGuard> ExecuteInternal() override;
        void GpuWaitOn(GpuGuard* guard) override;

        void TransitionResource(RenderResource* resource, ResourceState state) override;
        void FlushResourceBarriers() override;
        void FlushAllPending() override;

        void PushRenderTarget(RenderResource* color_target, uint32_t index = 0) override;
        void PopRenderTarget(uint32_t index = 0) override;
        void SetDepthStencil(RenderResource* ds_target) override;
        void ClearRenderTargets() override;

        void SetShaderResourceInput(RenderResource* resource, uint32_t slot) override;
        void SetRandomReadWriteInput(RenderResource* resource, uint32_t slot) override;
        void ClearShaderResource(uint32_t slot) override;

        void SetConstantShaderData(uint32_t slot, const void* data, uint32_t data_size) override;

        void SetVertexShader(uint32_t shader_index) override;
        void SetPixelShader(uint32_t shader_index) override;
        void SetComputeShader(uint32_t shader_index) override;

        void Clear(RenderResource* resource, const Math::Float4& color) override;

        void SetViewport(const Viewport& viewport) override;
        void SetViewports(const Viewport* viewports, uint32_t total_viewports) override;

        void SetScissor(const Viewport& scissor) override;
        void SetScissors(const Viewport* scissors, uint32_t total_scissors) override;

        void SetBlendMode(const BlendMode& mode) override;
        void SetCullMode(const CullMode& mode) override;
        void SetDepthMode(const DepthMode& mode, bool write_depth, bool reversed_depth) override;

        void SetIndexBuffer(RenderResource* index_resource) override;
        void SetVertexBuffer(RenderResource* vertex_resource, uint32_t slot) override;
        void SetVertexBuffers(RenderResource** vertex_resources, uint32_t resource_count, uint32_t start_slot) override;

        void CopyResource(RenderResource* src, RenderResource* dst) override;

        void UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size) override;

        void DrawInternal() override;
        void DrawInstancedInternal(uint32_t instances) override;
        void DispatchInternal(uint32_t thread_groups_x, uint32_t thread_groups_y, uint32_t thread_groups_z) override;

        void ProfileMarkerBegin(uint64_t color, const char* name) override;
        void ProfileMarkerEnd() override;

        void* GetNativeInterface() const override { return m_CommandList.Get(); }

    private:
        void PrepareDraw();

        void HandlePendingClears();
        void InternalCopy(GpuResource* src, GpuResource* dst, const RenderResourceType& primitive_type);

        void SetRenderTargets();

        void BindDescriptorHeaps();
        void BindResources(bool compute);
        void ClearResources();

        void SetGraphicsPipelineState();
        void SetComputePipelineState();
        void SetNewCommandList();

        bool                                m_CopyOperationsOnly;
        DeviceQueue*                        m_Queue;
        GPtr<ID3D12GraphicsCommandList2>    m_CommandList;

        struct PendingClear
        {
            bool                        renderTarget;
            D3D12_CPU_DESCRIPTOR_HANDLE handle;
            Math::Float4                color;
        };

        struct RenderState
        {
            bool                                psoDirty = true;
            bool                                rootSignatureDirty = true;

            GPtr<ID3D12RootSignature>           rootSignature = nullptr;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE       vertexBufferType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
            uint32_t                            vertexBufferCount = 0;
            uint32_t                            vertexCountPerInstance = 0;
            uint32_t                            indexCountPerInstance = 0;
            bool                                scissorSet = false;
            bool                                viewportSet = false;
            uint32_t                            width = 0;
            uint32_t                            height = 0;
            Stack<D3D12_CPU_DESCRIPTOR_HANDLE>  rtvHandles[8];
            Stack<DXGI_FORMAT>                  rtvFormats[8];
            D3D12_CPU_DESCRIPTOR_HANDLE         dsvHandle;
            DXGI_FORMAT                         dsvFormat = DXGI_FORMAT_UNKNOWN;
            uint32_t                            numRenderTargets = 0;
            bool                                renderTargetDirty = true;
            int32_t                             vsShader = -1;
            int32_t                             psShader = -1;
            int32_t                             csShader = -1;
            bool                                blendingSet = false;
            D3D12_BLEND_DESC                    blendDesc = {};
            bool                                rasterizerSet = false;
            D3D12_RASTERIZER_DESC               rasterizerDesc = {};
            bool                                depthStencilSet = false;
            D3D12_DEPTH_STENCIL_DESC            depthStencilDesc = {};
            D3D12_GPU_VIRTUAL_ADDRESS           cbvAddresses[16];

            DescriptorIndex                     shaderResourceHandles[SHADER_RESOURCE_SLOTS];

            List<PendingClear>                  pendingClears;
        };

        RenderState                             m_RenderState;
    };
}
#endif