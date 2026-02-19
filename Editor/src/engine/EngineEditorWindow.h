#pragma once

// We need the windows define to be able to access the windows platform files inside RabBit
#define RB_PLATFORM_WINDOWS 1

#include "platform/windowing/windows/WindowWin.h"
#include "imgui.h"

namespace Editor
{
    class EngineEditorWindow : public RB::Graphics::Windows::WindowWin
    {
    public:
        EngineEditorWindow(const char* name);
        ~EngineEditorWindow();

        // Call before trying to do any ImGui stuff for this window
        void Select();
        void SelectForDraw();
        void DeselectForDraw();

        void Update() override;

        void SetBorderless(bool borderless) override;

    private:
        void DestroyWindow() override;

        const char*   m_Name;
        std::string   m_TitleWindowName;
        ImGuiContext* m_Context;
    };
}