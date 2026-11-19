#pragma once

#include "WindowPanel.h"
#include <RabBit.h>
#include <string>

namespace Editor
{
    // PIX-style timeline of the last resolved GPU frame, scaled to always fill the window width
    class GpuTimelinePanel : public WindowPanel
    {
    public:
        GpuTimelinePanel(int window_flags);

        void OnCreate() override;
        void OnDestroy() override;
        void OnUpdate() override;

    private:
        struct Segment
        {
            std::string name;
            int         depth;
            float       beginMs;
            float       endMs;
        };

        struct QueueSnapshot
        {
            std::string       name;
            float             totalMs  = 0.0f;
            float             idleMs   = 0.0f;
            int               maxDepth = 0;
            RB::List<Segment> segments;
            RB::List<Segment> idles;  // Gaps between top-level queries, always at depth 0
        };

        void TakeSnapshot();
        void DrawRuler(float total_ms, float width);
        void DrawQueue(const QueueSnapshot& queue, float total_ms, float width);

        QueueSnapshot m_Queues[RB::Graphics::GpuTimer::kMaxQueues];
        bool          m_Paused;
    };
}
