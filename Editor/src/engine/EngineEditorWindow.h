#pragma once

#include "panels/WindowPanel.h"

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

        template<class T, typename... Args>
        T* AddPanel(Args&&... args);

        void RemovePanel(WindowPanel* panel);

    private:
        void UpdatePanels();
        void DestroyWindow() override;

        const char*             m_Name;
        std::string             m_TitleWindowName;
        RB::List<WindowPanel*>  m_Panels;
        ImGuiContext*           m_Context;
        ImGuiID                 m_DockSpaceID;
    };

    template<class T, typename... Args>
    inline T* EngineEditorWindow::AddPanel(Args&&... args)
    {
        RB_STATIC_ASSERT(std::is_base_of_v<WindowPanel, T>, "T must derive from WindowPanel");

        T* panel = new T(std::forward<Args>(args)...);
        m_Panels.push_back(panel);
        panel->OnCreate();
        return panel;
    }

    inline void EngineEditorWindow::RemovePanel(WindowPanel* panel)
    {
        auto itr = std::find(m_Panels.begin(), m_Panels.end(), panel);
        if (itr != m_Panels.end())
        {
            panel->OnDestroy();
            m_Panels.erase(itr);
            delete panel;
        }
    }
}