#pragma once

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    // SRV & UAV descriptors
    #define BINDLESS_REGULAR_DESCRIPTORS		        10000
    #define BINDLESS_TRANSIENT_DESCRIPTORS_PER_CYCLE    50
    
    // RTV & DSV descriptors
    #define RENDERTARGET_REGULAR_DESCRIPTORS	        1000
    #define DEPTHSTENCIL_REGULAR_DESCRIPTORS	        1000
    #define RTV_DSV_TRANSIENT_DESCRIPTORS_PER_CYCLE     50

    #define DESCRIPTOR_HEAP_TRANSIENT_CYCLES            3

    enum class DescriptorHandleType
    {
        SRV,
        UAV,
        CBV,
        RTV,
        DSV,
        Sampler
    };

    struct DescriptorIndex
    {
        int32_t                 heapIndex = -1;
        DescriptorHandleType    type;
        bool                    transient;

        bool isValid() const { return heapIndex >= 0; }
    };

    class DescriptorHeap;

    class DescriptorManager
    {
    public:
        DescriptorManager();
        ~DescriptorManager();

        // A transient descriptor will automatically be free'd/reused over time
        DescriptorIndex CreateDescriptor(ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, bool transient = false);
        DescriptorIndex CreateDescriptor(ID3D12Resource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, bool transient = false);
        DescriptorIndex CreateDescriptor(ID3D12Resource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc, bool transient = false);
        DescriptorIndex CreateDescriptor(ID3D12Resource* res, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, bool transient = false);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const DescriptorIndex& idx);
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const DescriptorIndex& idx);

        void InvalidateDescriptor(DescriptorIndex& idx);

        void CycleDescriptors();

        Array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> GetHeaps(uint32_t& num_heaps);

        DescriptorIndex GetDummyRwTex2DHandle() const { return m_DummyRwTex2DHandle; }

    private:
        DescriptorHeap* m_BindlessSrvUavHeap;
        DescriptorHeap* m_RenderTargetHeap;
        DescriptorHeap* m_DepthStencilHeap;

        DescriptorIndex m_DummyRwTex2DHandle;
    };

    extern DescriptorManager* g_DescriptorManager;

    // ----------------------------------------------------------------------------------

    class DescriptorHeap
    {
    public:
        DescriptorHeap(const wchar_t* name, bool shader_visible, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t max_persistent_descriptors, uint32_t max_transient_descriptors_per_cycle);

        int32_t AllocPersistent();
        int32_t AllocTransient();

        void CycleTransientDescriptors();

        void InvalidateDescriptor(int32_t& heap_index);

        GPtr<ID3D12DescriptorHeap> GetHeap() const { return m_Heap; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_Type; }

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(int32_t offset) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(int32_t offset) const;

    private:
        GPtr<ID3D12DescriptorHeap>	m_Heap;
        D3D12_CPU_DESCRIPTOR_HANDLE m_CpuStart;
        D3D12_GPU_DESCRIPTOR_HANDLE m_GpuStart;
        D3D12_DESCRIPTOR_HEAP_TYPE	m_Type;
        bool						m_ShaderVisible;
        uint32_t					m_IncrementSize;

        List<bool>					m_PersistentSlots;
        uint32_t					m_MaxPersistent;
        uint32_t					m_MaxTransientPerCycle;
        uint32_t                    m_CurrPersistentIdx;
        uint32_t                    m_CurrTransientIdx;
        uint32_t                    m_TransientBase;
        uint32_t                    m_CycleIndex;
    };


}