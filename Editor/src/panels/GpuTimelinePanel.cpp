#include "GpuTimelinePanel.h"
#include "imgui.h"

#include <cmath>
#include <functional>

using namespace RB;
using namespace RB::Graphics;

namespace Editor
{
    static constexpr float c_RowHeight   = 22.0f;
    static constexpr float c_RulerHeight = 22.0f;
    static constexpr float c_TickSpacing = 90.0f;   // Desired pixels between ruler ticks

    // Picks a 1/2/5 * 10^n step so ticks end up roughly c_TickSpacing pixels apart
    static float GetTickStep(float total_ms, float width)
    {
        float raw_step = c_TickSpacing * total_ms / width;
        float pow10    = Math::Pow(10.0f, Math::Floor(std::log10(raw_step)));
        float norm     = raw_step / pow10;

        if (norm < 1.5f) return pow10;
        if (norm < 3.5f) return 2.0f * pow10;
        if (norm < 7.5f) return 5.0f * pow10;
        return 10.0f * pow10;
    }

    // Stable color per pass name, so a pass keeps its color across frames
    static ImU32 GetSegmentColor(const std::string& name)
    {
        size_t hash = std::hash<std::string>{}(name);
        float  hue  = (float)(hash % 360) / 360.0f;

        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(hue, 0.55f, 0.65f, r, g, b);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
    }

    // Draws the label centered in the box when it fits, otherwise left aligned and clipped to the box
    static void DrawLabel(ImDrawList* draw_list, ImVec2 min, ImVec2 max, ImU32 color, const char* text)
    {
        float width = max.x - min.x;
        if (width < 8.0f)
        {
            return;
        }

        ImVec2 text_size = ImGui::CalcTextSize(text);
        float  x = text_size.x + 8.0f <= width ? min.x + (width - text_size.x) * 0.5f : min.x + 4.0f;
        float  y = min.y + (max.y - min.y - text_size.y) * 0.5f;

        draw_list->PushClipRect(ImVec2(min.x + 2.0f, min.y), ImVec2(max.x - 2.0f, max.y), true);
        draw_list->AddText(ImVec2(x, y), color, text);
        draw_list->PopClipRect();
    }

    GpuTimelinePanel::GpuTimelinePanel(int window_flags)
        : WindowPanel(window_flags)
        , m_Paused(false)
    {
    }

    void GpuTimelinePanel::OnCreate()
    {
    }

    void GpuTimelinePanel::OnDestroy()
    {
    }

    void GpuTimelinePanel::OnUpdate()
    {
        if (!m_Paused)
        {
            TakeSnapshot();
        }

        ImGui::Begin("GPU Timeline", nullptr, (ImGuiWindowFlags)m_WindowFlags);

        // All queues share one time axis, so their timelines line up
        float total_ms = 0.0f;
        for (const QueueSnapshot& queue : m_Queues)
        {
            total_ms = Math::Max(total_ms, queue.totalMs);
        }

        ImGui::Checkbox("Pause", &m_Paused);
        ImGui::SameLine();
        ImGui::Text("GPU frame: %.2f ms", total_ms);
        ImGui::Separator();

        float width = ImGui::GetContentRegionAvail().x;

        if (total_ms <= 0.0f || width < 1.0f)
        {
            ImGui::TextDisabled("No GPU timing data available");
            ImGui::End();
            return;
        }

        DrawRuler(total_ms, width);

        for (const QueueSnapshot& queue : m_Queues)
        {
            if (!queue.segments.empty())
            {
                DrawQueue(queue, total_ms, width);
            }
        }

        ImGui::End();
    }

    void GpuTimelinePanel::TakeSnapshot()
    {
        GpuTimer*       timer     = Application::GetInstance()->GetRenderer()->GetGpuTimer();
        FrameAllocator* allocator = Application::GetInstance()->GetAllocator();

        for (uint32_t queue_index = 0; queue_index < GpuTimer::kMaxQueues; ++queue_index)
        {
            GpuTimeline           timeline = timer->GetLastFrame(queue_index, allocator);
            Span<GpuTimelineNode> nodes    = timeline.timestamps;
            QueueSnapshot&        out      = m_Queues[queue_index];

            out.segments.clear();
            out.idles.clear();
            out.maxDepth = 0;
            out.totalMs  = timeline.totalMs;
            out.idleMs   = timeline.totalIdleTime;
            out.name     = nodes.empty() ? std::string() : std::string(nodes[0].name);

            if (nodes.size() <= 1)
            {
                continue;
            }

            // Parents always come before their children, so depths can be computed in a single pass.
            // The root has depth -1, which puts the top-level queries at depth 0.
            List<int> depths(nodes.size(), -1);
            for (size_t i = 1; i < nodes.size(); ++i)
            {
                const GpuTimelineNode& node = nodes[i];
                depths[i] = depths[node.parentIdx] + 1;
                out.maxDepth = Math::Max(out.maxDepth, depths[i]);

                out.segments.push_back({ std::string(node.name), depths[i], node.beginSec * 1000.0f, node.endSec * 1000.0f });
            }

            // Same definition as the GpuTimer: idle time is the gaps between the top-level queries
            float frame_end = 0.0f;
            for (int idx = nodes[0].firstChildIdx; idx != -1; idx = nodes[idx].nextSiblingIdx)
            {
                float begin = nodes[idx].beginSec * 1000.0f;
                float end   = nodes[idx].endSec * 1000.0f;

                if (begin > frame_end)
                {
                    out.idles.push_back({ "Idle", 0, frame_end, begin });
                }
                frame_end = Math::Max(frame_end, end);
            }
        }
    }

    void GpuTimelinePanel::DrawRuler(float total_ms, float width)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2      origin    = ImGui::GetCursorScreenPos();
        float       step      = GetTickStep(total_ms, width);
        float       px_per_ms = width / total_ms;

        const char* format = step >= 1.0f ? "%.0f ms" : (step >= 0.1f ? "%.1f ms" : "%.2f ms");

        draw_list->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + c_RulerHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));

        for (int i = 0; ; ++i)
        {
            float ms = step * i;
            if (ms > total_ms + step * 0.001f)
            {
                break;
            }

            float x = origin.x + ms * px_per_ms;
            draw_list->AddLine(ImVec2(x, origin.y + c_RulerHeight * 0.5f), ImVec2(x, origin.y + c_RulerHeight), ImGui::GetColorU32(ImGuiCol_Text));

            char label[32];
            snprintf(label, sizeof(label), format, ms);

            // Skip labels that would stick out of the right side of the view
            float label_width = ImGui::CalcTextSize(label).x;
            if (x + 3.0f + label_width <= origin.x + width)
            {
                draw_list->AddText(ImVec2(x + 3.0f, origin.y + 2.0f), ImGui::GetColorU32(ImGuiCol_Text), label);
            }
        }

        ImGui::Dummy(ImVec2(width, c_RulerHeight));
    }

    void GpuTimelinePanel::DrawQueue(const QueueSnapshot& queue, float total_ms, float width)
    {
        ImGui::Text("%s", queue.name.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("busy %.2f ms | idle %.2f ms", queue.totalMs - queue.idleMs, queue.idleMs);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2      origin    = ImGui::GetCursorScreenPos();
        float       height    = (queue.maxDepth + 1) * c_RowHeight;
        float       px_per_ms = width / total_ms;

        ImGui::PushID(queue.name.c_str());
        ImGui::InvisibleButton("##lane", ImVec2(width, height));
        bool lane_hovered = ImGui::IsItemHovered();
        ImGui::PopID();

        ImVec2 lane_max(origin.x + width, origin.y + height);
        draw_list->AddRectFilled(origin, lane_max, IM_COL32(25, 25, 25, 255));

        // Faint grid lines matching the ruler ticks
        float step = GetTickStep(total_ms, width);
        for (float ms = step; ms < total_ms; ms += step)
        {
            float x = origin.x + ms * px_per_ms;
            draw_list->AddLine(ImVec2(x, origin.y), ImVec2(x, lane_max.y), IM_COL32(255, 255, 255, 20));
        }

        auto get_rect = [&](const Segment& segment, ImVec2& min, ImVec2& max)
        {
            min = ImVec2(origin.x + segment.beginMs * px_per_ms, origin.y + segment.depth * c_RowHeight + 1.0f);
            max = ImVec2(origin.x + segment.endMs * px_per_ms,   min.y + c_RowHeight - 2.0f);
        };

        const Segment* hovered = nullptr;
        ImVec2         mouse   = ImGui::GetIO().MousePos;
        char           label[256];

        // Idle gaps: dark, hatched boxes on the top-level row
        draw_list->PushClipRect(origin, lane_max, true);
        for (const Segment& idle : queue.idles)
        {
            ImVec2 min, max;
            get_rect(idle, min, max);

            draw_list->AddRectFilled(min, max, IM_COL32(45, 45, 45, 255));

            draw_list->PushClipRect(min, max, true);
            for (float x = min.x - c_RowHeight; x < max.x; x += 6.0f)
            {
                draw_list->AddLine(ImVec2(x, max.y), ImVec2(x + c_RowHeight, min.y), IM_COL32(90, 90, 90, 255));
            }
            draw_list->PopClipRect();

            snprintf(label, sizeof(label), "Idle (%.2f ms)", idle.endMs - idle.beginMs);
            DrawLabel(draw_list, min, max, IM_COL32(200, 200, 200, 255), label);

            if (lane_hovered && mouse.x >= min.x && mouse.x < max.x && mouse.y >= min.y && mouse.y < max.y)
            {
                hovered = &idle;
            }
        }

        // Timed passes
        for (const Segment& segment : queue.segments)
        {
            ImVec2 min, max;
            get_rect(segment, min, max);

            // Keep very short passes visible
            max.x = Math::Max(max.x, min.x + 1.0f);

            draw_list->AddRectFilled(min, max, GetSegmentColor(segment.name));
            draw_list->AddRect(min, max, IM_COL32(0, 0, 0, 140));

            snprintf(label, sizeof(label), "%s (%.2f ms)", segment.name.c_str(), segment.endMs - segment.beginMs);
            DrawLabel(draw_list, min, max, IM_COL32(255, 255, 255, 255), label);

            if (lane_hovered && mouse.x >= min.x && mouse.x < max.x && mouse.y >= min.y && mouse.y < max.y)
            {
                hovered = &segment;
            }
        }
        draw_list->PopClipRect();

        if (hovered)
        {
            float duration = hovered->endMs - hovered->beginMs;

            ImGui::BeginTooltip();
            ImGui::Text("%s", hovered->name.c_str());
            ImGui::Separator();
            ImGui::Text("Duration: %.3f ms (%.1f%% of frame)", duration, duration / total_ms * 100.0f);
            ImGui::Text("Start:    %.3f ms", hovered->beginMs);
            ImGui::Text("End:      %.3f ms", hovered->endMs);
            ImGui::EndTooltip();
        }

        ImGui::Spacing();
    }
}
