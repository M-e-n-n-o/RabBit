#include "ConsolePanel.h"
#include "imgui.h"

using namespace RB;

namespace Editor
{
    const uint32_t ConsolePanel::c_MaxCharacters = 25000;

    ConsolePanel::ConsolePanel(int window_flags)
        : WindowPanel(window_flags)
    {
        m_Text.reserve(c_MaxCharacters);
    }

    void ConsolePanel::OnCreate()
    {
    }

    void ConsolePanel::OnDestroy()
    {
    }

    void ConsolePanel::OnUpdate()
    {
        ImGui::Begin("Console", nullptr, (ImGuiWindowFlags)m_WindowFlags);

        if (ImGui::Button("Clear"))
        {
            m_Text.clear();
        }

        ImGui::TextUnformatted(m_Text.c_str(), m_Text.end()._Ptr);

        ImGui::End();
    }

    void ConsolePanel::AppendLog(int mode, const char* text)
    {
        m_Text += text;

        if (m_Text.size() > c_MaxCharacters)
            m_Text.erase(0, m_Text.size() - c_MaxCharacters);
    }
}