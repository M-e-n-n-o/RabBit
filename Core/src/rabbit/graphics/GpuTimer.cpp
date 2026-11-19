#include "RabBitCommon.h"
#include "GpuTimer.h"
#include "RenderResource.h"

namespace RB::Graphics
{
    static constexpr const char* kQueueNames[] = { "Queue 0", "Queue 1", "Queue 2" };
    static_assert(std::size(kQueueNames) == GpuTimer::kMaxQueues, "Every queue needs a name");

    GpuTimer::GpuTimer()
    {
        ResetTree();

        for (uint32_t queue = 0; queue < kMaxQueues; ++queue)
        {
            GpuTimelineNode root = m_Tree[queue];
            root.nextSiblingIdx = -1;
            root.openIdx        = 0;

            m_LastTrees[queue].push_back(root);
            m_LastTotalMs[queue] = 0.0f;
            m_LastIdleMs[queue]  = 0.0f;
        }
    }

    void GpuTimer::Update()
    {
        RB_MUTEX_AUTO_LOCK(m_Mutex);

        if (m_Tree.size() == kMaxQueues)
        {
            return;
        }

        for (uint32_t queue = 0; queue < kMaxQueues; ++queue)
        {
            RB_ASSERT_FATAL(LOGTAG_GRAPHICS, m_Tree[queue].openIdx == (int)queue, "Not all timestamp queries were ended this frame");
        }

        // The readable timestamps are from a few frames ago, but the query set only changes on a render graph change,
        // so the tree of the last recorded frame matches them. Right after such a change the results are briefly invalid.
        ResolveTree();
        ResetTree();
    }

    void GpuTimer::OnBegin(const Shared<TimestampQuery>& query, uint32_t queue)
    {
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, queue < kMaxQueues, "Queue index out of range");

        RB_MUTEX_AUTO_LOCK(m_Mutex);

        int parent_idx = m_Tree[queue].openIdx;
        int new_idx    = (int)m_Tree.size();

        GpuTimelineNode new_node = {};
        new_node.name           = query->GetName();
        new_node.parentIdx      = parent_idx;
        new_node.nextSiblingIdx = -1;
        new_node.firstChildIdx  = -1;
        new_node.lastChildIdx   = -1;
        new_node.openIdx        = -1;
        new_node.beginSec       = 0.0f;
        new_node.endSec         = 0.0f;

        m_Tree.push_back(new_node);

        GpuTimelineNode& parent = m_Tree[parent_idx];
        if (parent.lastChildIdx == -1)
        {
            parent.firstChildIdx = new_idx;
        }
        else
        {
            m_Tree[parent.lastChildIdx].nextSiblingIdx = new_idx;
        }
        parent.lastChildIdx = new_idx;

        m_NodeQueries.push_back(query);
        m_Tree[queue].openIdx = new_idx;
    }

    void GpuTimer::OnEnd(const Shared<TimestampQuery>& query, uint32_t queue)
    {
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, queue < kMaxQueues, "Queue index out of range");

        RB_MUTEX_AUTO_LOCK(m_Mutex);

        GpuTimelineNode& root = m_Tree[queue];
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, m_NodeQueries[root.openIdx] == query,
            "Timestamp queries must be properly nested, only the innermost open query on a queue can be ended");

        root.openIdx = m_Tree[root.openIdx].parentIdx;
    }

    float GpuTimer::GetLastGpuTime() const
    {
        RB_MUTEX_AUTO_LOCK(m_Mutex);

        float max_ms = 0.0f;
        for (uint32_t queue = 0; queue < kMaxQueues; ++queue)
        {
            max_ms = std::max(max_ms, m_LastTotalMs[queue]);
        }
        return max_ms;
    }

    GpuTimeline GpuTimer::GetLastFrame(uint32_t queue, FrameAllocator* allocator) const
    {
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, queue < kMaxQueues, "Queue index out of range");

        RB_MUTEX_AUTO_LOCK(m_Mutex);

        const List<GpuTimelineNode>& tree = m_LastTrees[queue];

        GpuTimelineNode* nodes = allocator->Allocate<GpuTimelineNode>(tree.size());
        std::copy(tree.begin(), tree.end(), nodes);

        GpuTimeline timeline = {};
        timeline.totalMs       = m_LastTotalMs[queue];
        timeline.totalIdleTime = m_LastIdleMs[queue];
        timeline.timestamps    = Span<GpuTimelineNode>(nodes, tree.size());
        return timeline;
    }

    void GpuTimer::ResetTree()
    {
        m_Tree.clear();
        m_NodeQueries.assign(kMaxQueues, nullptr);

        for (uint32_t queue = 0; queue < kMaxQueues; ++queue)
        {
            GpuTimelineNode root = {};
            root.name           = kQueueNames[queue];
            root.parentIdx      = -1;
            root.nextSiblingIdx = queue + 1 < kMaxQueues ? (int)queue + 1 : -1;
            root.firstChildIdx  = -1;
            root.lastChildIdx   = -1;
            root.openIdx        = (int)queue;
            root.beginSec       = 0.0f;
            root.endSec         = 0.0f;

            m_Tree.push_back(root);
        }
    }

    void GpuTimer::ResolveTree()
    {
        for (uint32_t queue = 0; queue < kMaxQueues; ++queue)
        {
            List<GpuTimelineNode>& out = m_LastTrees[queue];
            out.clear();

            GpuTimelineNode root = m_Tree[queue];
            root.parentIdx      = -1;
            root.nextSiblingIdx = -1;
            root.firstChildIdx  = -1;
            root.lastChildIdx   = -1;
            root.openIdx        = 0;
            root.beginSec       = 0.0f;
            root.endSec         = 0.0f;
            out.push_back(root);

            m_LastTotalMs[queue] = 0.0f;
            m_LastIdleMs[queue]  = 0.0f;

            int first_top_level = m_Tree[queue].firstChildIdx;
            if (first_top_level == -1)
            {
                continue;
            }

            // Absolute timestamps are huge, so only convert to float once they are relative to the frame
            double frame_begin, unused;
            m_NodeQueries[first_top_level]->GetLatestStamps(frame_begin, unused);

            // Nodes are stored in begin order, and queries on a queue are strictly nested, so walking the
            // indices in order visits this queue's nodes depth first. That lets us copy them in one linear pass.
            // openIdx is unused on regular nodes, so it temporarily stores the node's index in the output (-1 = other queue).
            for (int idx = kMaxQueues; idx < (int)m_Tree.size(); ++idx)
            {
                GpuTimelineNode& src    = m_Tree[idx];
                int              parent = src.parentIdx;

                bool on_queue = parent < (int)kMaxQueues ? parent == (int)queue : m_Tree[parent].openIdx != -1;
                if (!on_queue)
                {
                    src.openIdx = -1;
                    continue;
                }

                int dst_idx    = (int)out.size();
                int dst_parent = parent < (int)kMaxQueues ? 0 : m_Tree[parent].openIdx;
                src.openIdx    = dst_idx;

                double begin, end;
                m_NodeQueries[idx]->GetLatestStamps(begin, end);

                GpuTimelineNode node = {};
                node.name           = src.name;
                node.parentIdx      = dst_parent;
                node.nextSiblingIdx = -1;
                node.firstChildIdx  = -1;
                node.lastChildIdx   = -1;
                node.openIdx        = -1;
                node.beginSec       = (float)(begin - frame_begin);
                node.endSec         = (float)(end - frame_begin);
                out.push_back(node);

                GpuTimelineNode& dst_parent_node = out[dst_parent];
                if (dst_parent_node.lastChildIdx == -1)
                {
                    dst_parent_node.firstChildIdx = dst_idx;
                }
                else
                {
                    out[dst_parent_node.lastChildIdx].nextSiblingIdx = dst_idx;
                }
                dst_parent_node.lastChildIdx = dst_idx;
            }

            // Top-level queries on one queue execute in submission order, gaps between them are idle time
            float frame_end = 0.0f;
            float busy      = 0.0f;

            for (int idx = out[0].firstChildIdx; idx != -1; idx = out[idx].nextSiblingIdx)
            {
                float begin = std::max(out[idx].beginSec, frame_end);
                float end   = out[idx].endSec;
                if (end > begin)
                {
                    busy += end - begin;
                }
                frame_end = std::max(frame_end, end);
            }

            out[0].endSec        = frame_end;
            m_LastTotalMs[queue] = frame_end * 1000.0f;
            m_LastIdleMs[queue]  = (frame_end - busy) * 1000.0f;
        }
    }
}
