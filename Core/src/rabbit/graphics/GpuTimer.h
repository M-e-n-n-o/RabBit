#pragma once

#include "app/FrameAllocator.h"

namespace RB::Graphics
{
    class TimestampQuery;

    struct GpuTimelineNode
    {
        std::string      name;
        int              parentIdx;
        int              nextSiblingIdx;
        int              firstChildIdx;
        int              lastChildIdx;
        int              openIdx;
        float            beginSec;
        float            endSec;
    };

    struct GpuTimeline
    {
        float                 totalMs;
        float                 totalIdleTime;
        Span<GpuTimelineNode> timestamps;   // timestamps[0] is the queue root
    };

    class GpuTimer
    {
    public:
        static constexpr uint32_t kMaxQueues = 3;

        GpuTimer();

        void Update();

        void OnBegin(const Shared<TimestampQuery>& query, uint32_t queue);
        void OnEnd(const Shared<TimestampQuery>& query, uint32_t queue);

        float       GetLastGpuTime() const;
        GpuTimeline GetLastFrame(uint32_t queue, FrameAllocator* allocator) const;

    private:
        void ResetTree();
        void ResolveTree();

    private:
        mutable Mutex                 m_Mutex;

        // Tree of the frame currently being recorded
        List<GpuTimelineNode>         m_Tree;
        List<Shared<TimestampQuery>>  m_NodeQueries;

        // Latest frame with resolved timestamps
        List<GpuTimelineNode>         m_LastTrees[kMaxQueues];
        float                         m_LastTotalMs[kMaxQueues];
        float                         m_LastIdleMs[kMaxQueues];
    };
}
