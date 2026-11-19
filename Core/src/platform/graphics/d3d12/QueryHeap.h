#if RB_GRAPHICS_API_D3D12

#pragma once

#include "graphics/RenderResource.h"
#include "platform/graphics/d3d12/RendererD3D12.h"
#include "platform/utils/Windows.h"

#include <d3d12.h>
#include <cstdint>

namespace RB::Graphics::D3D12
{
    class TimestampHeap
    {
    public:
        TimestampHeap(uint32_t max_queries, uint64_t queue_frequency);
        ~TimestampHeap();

        // Reserves 2 indices, for begin and end
        uint32_t ReserveIndex();
        void FreeIndex(uint32_t idx);

        void GetTimestamps(uint32_t idx, double& begin, double& end);

        void OnBegin(ID3D12GraphicsCommandList* cmd_list, uint32_t idx);
        void OnEnd(ID3D12GraphicsCommandList* cmd_list, uint32_t idx);

        void Update();
        void Resolve(ID3D12GraphicsCommandList* cmd_list);

        GPtr<ID3D12QueryHeap> GetHeap()  const { return m_Heap; }

    private:
        ReadWriteLock                                               m_RWLock;
        GPtr<ID3D12QueryHeap>                                       m_Heap;
        uint32_t                                                    m_HeapCount;
        Queue<uint32_t>                                             m_FreeIndices;
        Array<Shared<ReadbackBuffer>, MAX_NUM_CPU_FRAMES_IN_FLIGHT> m_ReadbackBuffers;
        uint64_t*                                                   m_ReadbackData;
        List<bool>                                                  m_PendingResolve;
        uint32_t                                                    m_CycleIndex;
        double                                                      m_Frequency;
        bool                                                        m_Used;
    };

    // Need one per queue type due to different frequencies
    extern TimestampHeap* g_GraphicsTimestampHeap;
}
#endif