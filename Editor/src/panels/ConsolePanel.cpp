#include "ConsolePanel.h"
#include <RabBit.h>

using namespace RB;

namespace Editor
{
    const uint32_t ConsolePanel::c_MaxCharacters = 25000;

    ConsolePanel::ConsolePanel()
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
        ImGui::Begin("Console");

        if (ImGui::Button("Clear"))
        {
            m_Text.clear();
        }

        ImGui::TextUnformatted(m_Text.c_str(), m_Text.end()._Ptr);

        ImGui::End();
    }

    void ConsolePanel::AppendLog(int mode, const char* format, va_list args)
    {
        m_Text += FormatToString(format, args);

        if (m_Text.size() > c_MaxCharacters)
            m_Text.erase(0, m_Text.size() - c_MaxCharacters);
    }

    std::string ConsolePanel::FormatToString(const char* format, va_list args)
    {
        // Make a copy of args because vsnprintf consumes the va_list
        va_list args_copy;
        va_copy(args_copy, args);

        int size = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size <= 0)
        {
            return std::string();
        }

        std::string result;
        result.resize(size);

        vsnprintf(&result[0], size + 1, format, args);

        return result;
    }
}