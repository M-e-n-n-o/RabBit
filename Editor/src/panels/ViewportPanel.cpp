#include "ViewportPanel.h"
#include "imgui.h"

// We need these defines to be able to access the platform files inside RabBit
#define RB_GRAPHICS_API_D3D12 1
#define RB_PLATFORM_WINDOWS 1
#include <RabBit.h>
#include <rabbit/events/Event.h>
#include <rabbit/events/ApplicationEvent.h>

#include "platform/graphics/d3d12/resource/RenderResourceD3D12.h"
#include "platform/graphics/d3d12/resource/Descriptor.h"

using namespace RB;
using namespace RB::Graphics;

namespace Editor
{
    ViewportPanel::ViewportPanel()
        : m_Width(1280)
        , m_Height(720)
        , m_DeltaIndex(0)
    {
        m_SceneTexture = Texture2D::Create("Game scene", RenderResourceFormat::RGBA8_UNORM, m_Width, m_Height, true, true);

        for (int i = 0; i < c_HistoryLength; i++)
        {
            m_MainThreadAverages[i] = 0;
            m_RenderThreadAverages[i] = 0;
        }
    }

    void ViewportPanel::OnCreate()
    {

    }

    void ViewportPanel::OnDestroy()
    {

    }

    void ViewportPanel::OnUpdate()
    {
        ImGui::Begin("Viewport");
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();

        ImGui::Image((ImTextureID)(D3D12::g_DescriptorManager->GetGpuHandle((std::static_pointer_cast<D3D12::Texture2DD3D12>(m_SceneTexture)->GetSrvHandle())).ptr), ImVec2(m_Width, m_Height));

        if ((Math::Abs(size.x - m_Width) > 0.01f || Math::Abs(size.y - m_Height) > 0.01f) &&
            size.x > 1.0f && size.y > 1.0f)
        {
            m_Width = size.x;
            m_Height = size.y;

            Events::RenderOutputChangedEvent e;
            Events::g_EventManager->InsertEvent(e);

            // Recreate the texture with the new size
            m_SceneTexture = Texture2D::Create("Game scene", RenderResourceFormat::RGBA8_UNORM, m_Width, m_Height, true, true);
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();

        const ImVec2 graph_pos(pos.x + 10.0f, pos.y + 40.0f);
        const ImVec2 graph_size(200.0f, 80.0f);
        const ImVec2 panel_min(pos.x + 10.0f, pos.y + 10.0f);

        const float max_ms = 50.0f; // 20 FPS

        const ImU32 main_color = IM_COL32(0, 255, 0, 255);
        const ImU32 render_color = IM_COL32(255, 0, 0, 255);

        // Background
        dl->AddRectFilled(panel_min, ImVec2(graph_pos.x + graph_size.x, graph_pos.y + graph_size.y), IM_COL32(0, 0, 0, 127), 5.0f);

        float main_average = 0;
        float render_average = 0;
        {
            m_DeltaIndex = (m_DeltaIndex + 1) % c_HistoryLength;
            m_MainThreadAverages[m_DeltaIndex] = Application::GetInstance()->GetDeltaTime() * 1000.0f;
            m_RenderThreadAverages[m_DeltaIndex] = Application::GetInstance()->GetRenderer()->GetLastFrameTime();

            int count = Math::Min(50, c_HistoryLength);
            for (int i = 0; i < count; i++)
            {
                int idx = (m_DeltaIndex - i + c_HistoryLength) % c_HistoryLength;
                main_average += m_MainThreadAverages[idx];
                render_average += m_RenderThreadAverages[idx];
            }
            main_average /= count;
            render_average /= count;
        }

        dl->AddText(ImVec2(panel_min.x + 8, panel_min.y + 8), main_color, std::format("Main thread: {:.1f}ms", main_average).c_str());
        dl->AddText(ImVec2(panel_min.x + 8, panel_min.y + 20), render_color, std::format("Render thread: {:.1f}ms", render_average).c_str());

        auto PlotGraph = [&](float* timings, ImU32 color)
            {
                for (int i = 0; i < c_HistoryLength - 1; i++)
                {
                    int newest = m_DeltaIndex;
                    int idx0 = (newest - (c_HistoryLength - 1 - i) + c_HistoryLength) % c_HistoryLength;
                    int idx1 = (newest - (c_HistoryLength - 2 - i) + c_HistoryLength) % c_HistoryLength;

                    float x0 = graph_pos.x + (float)i / (c_HistoryLength - 1) * graph_size.x;
                    float x1 = graph_pos.x + (float)(i + 1) / (c_HistoryLength - 1) * graph_size.x;
                    float y0 = graph_pos.y + graph_size.y - (timings[idx0] / max_ms) * graph_size.y;
                    float y1 = graph_pos.y + graph_size.y - (timings[idx1] / max_ms) * graph_size.y;

                    dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), color, 1.0f);
                }
            };

        PlotGraph(m_MainThreadAverages, main_color);
        PlotGraph(m_RenderThreadAverages, render_color);

        auto DrawMarker = [&](float ms)
            {
                float y = graph_pos.y + graph_size.y - (ms / max_ms) * graph_size.y;

                dl->AddLine(ImVec2(graph_pos.x + 25, y), ImVec2(graph_pos.x + graph_size.x, y), IM_COL32(255, 255, 255, 50));
                dl->AddText(ImVec2(graph_pos.x + 2, y - 7), IM_COL32(255, 255, 255, 50), std::format("{}", int(1.0f / (ms / 1000.0f))).c_str());
            };

        DrawMarker(16.67f); // 60 FPS
        DrawMarker(8.33f);  // 120 FPS

        ImGui::End();
    }
}