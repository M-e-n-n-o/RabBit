#pragma once

// We need the windows define to be able to access the windows platform files inside RabBit
#define RB_PLATFORM_WINDOWS 1

#include "platform/windowing/windows/WindowWin.h"
#include "imgui.h"

namespace Editor
{
    class EditorWindow : public RB::Graphics::Windows::WindowWin
    {
    public:
        EditorWindow(const char* name);
        ~EditorWindow();

        // Call before trying to do any ImGui stuff for this window
        void Select();

        void Update() override;

        void SetBorderless(bool borderless) override;

    private:
        void DestroyWindow() override;

        ImGuiContext* m_Context;
    };
}