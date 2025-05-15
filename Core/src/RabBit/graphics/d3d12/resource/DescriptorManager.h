#pragma once

#include "DescriptorHeap.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    // Tex2D descriptors
    #define BINDLESS_TEX2D_DESCRIPTORS			10000
    #define BINDLESS_TEX2D_START_OFFSET			0
    
    // RwTex2D descriptors
    #define BINDLESS_RWTEX2D_DESCRIPTORS		1000
    #define BINDLESS_RWTEX2D_START_OFFSET		(BINDLESS_TEX2D_START_OFFSET + BINDLESS_TEX2D_DESCRIPTORS)
    
    #define BINDLESS_DESCRIPTOR_HEAP_SIZE		(BINDLESS_TEX2D_DESCRIPTORS + BINDLESS_RWTEX2D_DESCRIPTORS)
    
    // RTV & DSV descriptors
    #define RENDERTARGET_DESCRIPTOR_HEAP_SIZE	100
    #define DEPTHSTENCIL_DESCRIPTOR_HEAP_SIZE	100

    enum class DescriptorHandleType
    {
        SRV,
        UAV,
        CBV,
        RTV,
        DSV,
        Sampler
    };

    class DescriptorManager
    {
    public:
        DescriptorManager();
        ~DescriptorManager();

        DescriptorHandle CreateDescriptor(ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
        DescriptorHandle CreateDescriptor(ID3D12Resource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
        DescriptorHandle CreateDescriptor(ID3D12Resource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE& descriptor);
        DescriptorHandle CreateDescriptor(ID3D12Resource* res, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE& descriptor);

        void InvalidateDescriptor(DescriptorHandle& handle, const DescriptorHandleType& type);

        Array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> GetHeaps(uint32_t& num_heaps);

        D3D12_GPU_DESCRIPTOR_HANDLE GetSrvUavStartHandle() const;

    private:
        DescriptorHeap* m_BindlessSrvUavHeap;
        DescriptorHeap* m_RenderTargetHeap;
        DescriptorHeap* m_DepthStencilHeap;
    };

    extern DescriptorManager* g_DescriptorManager;
}