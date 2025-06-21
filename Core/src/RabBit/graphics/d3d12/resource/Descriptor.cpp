#include "RabBitCommon.h"
#include "Descriptor.h"
#include "graphics/d3d12/GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
    // ---------------------------------------------------------------------------
    //							    DescriptorManager
    // ---------------------------------------------------------------------------

    DescriptorManager* g_DescriptorManager = nullptr;

    DescriptorManager::DescriptorManager()
    {
        m_BindlessSrvUavHeap = new DescriptorHeap(L"Bindless SRV/UAV heap", true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, BINDLESS_REGULAR_DESCRIPTORS, BINDLESS_TRANSIENT_DESCRIPTORS_PER_CYCLE);
        m_RenderTargetHeap   = new DescriptorHeap(L"RenderTarget heap", false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RENDERTARGET_REGULAR_DESCRIPTORS, RTV_DSV_TRANSIENT_DESCRIPTORS_PER_CYCLE);
        m_DepthStencilHeap   = new DescriptorHeap(L"Depth Stencil heap", false, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DEPTHSTENCIL_REGULAR_DESCRIPTORS, RTV_DSV_TRANSIENT_DESCRIPTORS_PER_CYCLE);

        D3D12_UNORDERED_ACCESS_VIEW_DESC dummy_desc = {};
        dummy_desc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
        dummy_desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        dummy_desc.Texture2D.MipSlice   = 0;
        dummy_desc.Texture2D.PlaneSlice = 0;

        m_DummyRwTex2DHandle = CreateDescriptor(nullptr, dummy_desc);
    }

    DescriptorManager::~DescriptorManager()
    {
        SAFE_DELETE(m_BindlessSrvUavHeap);
        SAFE_DELETE(m_RenderTargetHeap);
        SAFE_DELETE(m_DepthStencilHeap);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetCpuHandle(const DescriptorIndex& idx)
    {
        switch (idx.type)
        {
        case DescriptorHandleType::SRV:
        case DescriptorHandleType::UAV: return m_BindlessSrvUavHeap->GetCpuHandle(idx.heapIndex);
        case DescriptorHandleType::RTV: return m_RenderTargetHeap->GetCpuHandle(idx.heapIndex);
        case DescriptorHandleType::DSV: return m_DepthStencilHeap->GetCpuHandle(idx.heapIndex);
        case DescriptorHandleType::CBV:
        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Descriptor handle type not valid");
            break;
        }

        return {};
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetGpuHandle(const DescriptorIndex& idx)
    {
        switch (idx.type)
        {
        case DescriptorHandleType::SRV:
        case DescriptorHandleType::UAV: return m_BindlessSrvUavHeap->GetGpuHandle(idx.heapIndex);
        case DescriptorHandleType::RTV: return m_RenderTargetHeap->GetGpuHandle(idx.heapIndex);
        case DescriptorHandleType::DSV: return m_DepthStencilHeap->GetGpuHandle(idx.heapIndex);
        case DescriptorHandleType::CBV:
        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Descriptor handle type not valid");
            break;
        }

        return {};
    }

    DescriptorIndex DescriptorManager::CreateDescriptor(ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, bool transient)
    {
        uint32_t heap_index = transient ? m_BindlessSrvUavHeap->AllocTransient() :
                                          m_BindlessSrvUavHeap->AllocPersistent();

        g_GraphicsDevice->Get()->CreateShaderResourceView(res, &desc, m_BindlessSrvUavHeap->GetCpuHandle(heap_index));

        DescriptorIndex di = {};
        di.heapIndex = heap_index;
        di.type      = DescriptorHandleType::SRV;
        di.transient = transient;

        return di;
    }

    DescriptorIndex DescriptorManager::CreateDescriptor(ID3D12Resource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, bool transient)
    {
        uint32_t heap_index = transient ? m_BindlessSrvUavHeap->AllocTransient() :
                                          m_BindlessSrvUavHeap->AllocPersistent();

        g_GraphicsDevice->Get()->CreateUnorderedAccessView(res, nullptr, &desc, m_BindlessSrvUavHeap->GetCpuHandle(heap_index));

        DescriptorIndex di = {};
        di.heapIndex = heap_index;
        di.type      = DescriptorHandleType::UAV;
        di.transient = transient;

        return di;
    }

    DescriptorIndex DescriptorManager::CreateDescriptor(ID3D12Resource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc, bool transient)
    {
        uint32_t heap_index = transient ? m_RenderTargetHeap->AllocTransient() :
                                          m_RenderTargetHeap->AllocPersistent();

        g_GraphicsDevice->Get()->CreateRenderTargetView(res, &desc, m_RenderTargetHeap->GetCpuHandle(heap_index));

        DescriptorIndex di = {};
        di.heapIndex = heap_index;
        di.type      = DescriptorHandleType::RTV;
        di.transient = transient;

        return di;
    }

    DescriptorIndex DescriptorManager::CreateDescriptor(ID3D12Resource* res, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, bool transient)
    {
        uint32_t heap_index = transient ? m_DepthStencilHeap->AllocTransient() :
                                          m_DepthStencilHeap->AllocPersistent();

        g_GraphicsDevice->Get()->CreateDepthStencilView(res, &desc, m_DepthStencilHeap->GetCpuHandle(heap_index));

        DescriptorIndex di = {};
        di.heapIndex = heap_index;
        di.type      = DescriptorHandleType::DSV;
        di.transient = transient;

        return di;
    }

    void DescriptorManager::InvalidateDescriptor(DescriptorIndex& idx)
    {
        switch (idx.type)
        {
        case DescriptorHandleType::SRV:
        case DescriptorHandleType::UAV: m_BindlessSrvUavHeap->InvalidateDescriptor(idx.heapIndex);  break;
        case DescriptorHandleType::RTV: m_RenderTargetHeap->InvalidateDescriptor(idx.heapIndex);    break;
        case DescriptorHandleType::DSV: m_DepthStencilHeap->InvalidateDescriptor(idx.heapIndex);    break;
        case DescriptorHandleType::CBV:
        default:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Descriptor handle type not valid");
            break;
        }
    }

    void DescriptorManager::CycleDescriptors()
    {
        m_BindlessSrvUavHeap->CycleTransientDescriptors();
        m_RenderTargetHeap->CycleTransientDescriptors();
        m_DepthStencilHeap->CycleTransientDescriptors();
    }

    Array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DescriptorManager::GetHeaps(uint32_t& num_heaps)
    {
        Array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> arr;

        arr[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = m_BindlessSrvUavHeap->GetHeap().Get();
        arr[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]     = nullptr;
        arr[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]         = nullptr;
        arr[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]         = nullptr;

        num_heaps = 1;

        return arr;
    }

    // ---------------------------------------------------------------------------
    //								DescriptorHeap
    // ---------------------------------------------------------------------------

    DescriptorHeap::DescriptorHeap(const wchar_t* name, bool shader_visible, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t max_persistent_descriptors, uint32_t max_transient_descriptors_per_cycle)
        : m_Type(type)
        , m_ShaderVisible(shader_visible)
        , m_MaxPersistent(max_persistent_descriptors)
        , m_MaxTransientPerCycle(max_transient_descriptors_per_cycle)
        , m_CurrPersistentIdx(0)
        , m_CycleIndex(0)
    {
        uint32_t max_descriptors = m_MaxPersistent + (m_MaxTransientPerCycle * DESCRIPTOR_HEAP_TRANSIENT_CYCLES);

        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, max_descriptors < 1e6, "The max total of descriptors cannot exceed 1 million");

        m_PersistentSlots.reserve(m_MaxPersistent);
        for (uint32_t i = 0; i < m_MaxPersistent; ++i)
        {
            m_PersistentSlots.push_back(true);
        }

        m_IncrementSize = g_GraphicsDevice->Get()->GetDescriptorHandleIncrementSize(m_Type);

        // Create descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC gpu_desc = {};
        gpu_desc.Type           = m_Type;
        gpu_desc.NumDescriptors = max_descriptors;
        gpu_desc.Flags          = m_ShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        gpu_desc.NodeMask       = 0;

        RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateDescriptorHeap(&gpu_desc, IID_PPV_ARGS(&m_Heap)),
            "Failed to create main descriptor heap: %ws", name);

        m_Heap->SetName(name);

        m_CpuStart = m_Heap->GetCPUDescriptorHandleForHeapStart();

        if (m_ShaderVisible)
        {
            m_GpuStart = m_Heap->GetGPUDescriptorHandleForHeapStart();
        }

        CycleTransientDescriptors();
    }

    int32_t DescriptorHeap::AllocPersistent()
    {
        uint32_t start = m_CurrPersistentIdx;
        uint32_t slot = start;

        // TODO This is slow, improve this!
        // Find the first available slot in the heap
        while (!m_PersistentSlots[slot])
        {
            slot = (slot + 1) % m_MaxPersistent;

            if (slot == start)
            {
                RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Increase the max amount of persistent descriptors for heap type: %d", (int)m_Type);
                return -1;
            }
        }

        m_PersistentSlots[slot] = false;

        uint32_t before = m_CurrPersistentIdx;
        m_CurrPersistentIdx = slot;

        return (int32_t)before;
    }

    int32_t DescriptorHeap::AllocTransient()
    {
        if ((m_CurrTransientIdx - m_TransientBase + 1) >= m_MaxTransientPerCycle)
        {
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Increase the max amount of transient descriptors for heap type: %d", (int)m_Type);
            m_CurrTransientIdx = m_TransientBase;
            return m_CurrTransientIdx;
        }

        uint32_t before = m_CurrTransientIdx;
        m_CurrTransientIdx++;

        return (int32_t)before;
    }

    void DescriptorHeap::CycleTransientDescriptors()
    {
        m_CycleIndex        = (m_CycleIndex + 1) % DESCRIPTOR_HEAP_TRANSIENT_CYCLES;
        m_TransientBase     = m_CycleIndex * m_MaxTransientPerCycle + m_MaxPersistent;
        m_CurrTransientIdx  = m_TransientBase;
    }

    void DescriptorHeap::InvalidateDescriptor(int32_t& heap_index)
    {
        if (heap_index < 0 || heap_index >= m_MaxPersistent)
        {
            // Invalid handle
            return;
        }

        m_PersistentSlots[heap_index] = true;
        heap_index = -1;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(int32_t offset) const
    {
        return D3D12_CPU_DESCRIPTOR_HANDLE{ m_CpuStart.ptr + offset * m_IncrementSize };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(int32_t offset) const
    {
        if (!m_ShaderVisible)
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Cannot get the GPU descriptor for a CPU visible descriptor heap");
            return {};
        }

        return D3D12_GPU_DESCRIPTOR_HANDLE{ m_GpuStart.ptr + offset * m_IncrementSize };
    }
}