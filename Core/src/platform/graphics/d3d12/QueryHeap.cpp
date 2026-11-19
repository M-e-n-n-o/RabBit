#if RB_GRAPHICS_API_D3D12

#include "RabBitCommon.h"
#include "QueryHeap.h"
#include "GraphicsDevice.h"
#include "resource/GpuResource.h"
#include "resource/RenderResourceD3D12.h"

namespace RB::Graphics::D3D12
{
    TimestampHeap* g_GraphicsTimestampHeap = nullptr;

    TimestampHeap::TimestampHeap(uint32_t max_queries, uint64_t queue_frequency)
        : m_Heap(nullptr)
        , m_CycleIndex(0)
        , m_Used(false)
        , m_Frequency(queue_frequency)
    {
        // One for both begin and end
        m_HeapCount = max_queries * 2;

        D3D12_QUERY_HEAP_DESC desc = {};
        desc.Type   = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        desc.Count  = m_HeapCount;

        RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateQueryHeap(&desc, IID_PPV_ARGS(&m_Heap)), 
            "Failed to create Timestamp Heap");

        for (int i = 0; i < max_queries; i++)
        {
            m_FreeIndices.push(i);
        }

        uint32_t readback_size = sizeof(uint64_t) * m_HeapCount;
        for (int i = 0; i < MAX_NUM_CPU_FRAMES_IN_FLIGHT; i++)
        {
            char name[25];
            sprintf(name, "Timestamp Readback %d", i);
            m_ReadbackBuffers[i] = ReadbackBuffer::Create(name, readback_size);
        }

        m_ReadbackData = (uint64_t*)ALLOC_HEAP(readback_size);
        memset(m_ReadbackData, 0, readback_size);

        m_PendingResolve.resize(m_HeapCount, false);
    }

    TimestampHeap::~TimestampHeap()
    {
        SAFE_FREE(m_ReadbackData);
    }

    uint32_t TimestampHeap::ReserveIndex()
    {
        if (m_FreeIndices.empty())
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Timestamp heap needs to be increased");
            return 0;
        }

        uint32_t idx = m_FreeIndices.front();
        m_FreeIndices.pop();
        return idx * 2;
    }

    void TimestampHeap::FreeIndex(uint32_t idx)
    {
        m_FreeIndices.push(idx / 2);
    }

    void TimestampHeap::GetTimestamps(uint32_t idx, double& begin, double& end)
    {
        RB_AUTO_READ_LOCK(m_RWLock);
        begin = m_ReadbackData[idx] / m_Frequency;
        end = m_ReadbackData[idx + 1] / m_Frequency;
    }

    void TimestampHeap::OnBegin(ID3D12GraphicsCommandList* cmd_list, uint32_t idx)
    {
        m_Used = true;
        m_PendingResolve[idx] = true;
        cmd_list->EndQuery(m_Heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, idx);
    }

    void TimestampHeap::OnEnd(ID3D12GraphicsCommandList* cmd_list, uint32_t idx)
    {
        m_Used = true;
        m_PendingResolve[idx + 1] = true;
        cmd_list->EndQuery(m_Heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, idx + 1);
    }

    void TimestampHeap::Update()
    {
        if (!m_Used)
        {
            return;
        }

        m_CycleIndex = (m_CycleIndex + 1) % MAX_NUM_CPU_FRAMES_IN_FLIGHT;
        m_Used = false;

        // The new cycle slot still holds the results of MAX_NUM_CPU_FRAMES_IN_FLIGHT frames ago,
        // which is the newest frame guaranteed to be finished on the GPU at this point.
        RB_AUTO_WRITE_LOCK(m_RWLock);
        m_ReadbackBuffers[m_CycleIndex]->GetDataDirect(m_ReadbackData);
    }

    void TimestampHeap::Resolve(ID3D12GraphicsCommandList* cmd_list)
    {
        if (!m_Used)
        {
            return;
        }

        ReadbackBufferD3D12* readback = (ReadbackBufferD3D12*)m_ReadbackBuffers[m_CycleIndex].get();
        GpuResource* res = (GpuResource*)readback->GetNativeResource();

        // Only resolve the indices that have been written, resolving a query that was never performed is invalid
        uint32_t idx = 0;
        while (idx < m_HeapCount)
        {
            if (!m_PendingResolve[idx])
            {
                idx++;
                continue;
            }

            uint32_t start = idx;
            while (idx < m_HeapCount && m_PendingResolve[idx])
            {
                m_PendingResolve[idx] = false;
                idx++;
            }

            cmd_list->ResolveQueryData(m_Heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, start, idx - start, res->GetResource(), start * sizeof(uint64_t));
        }
    }
}

#endif