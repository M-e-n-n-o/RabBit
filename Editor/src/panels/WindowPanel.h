#pragma once

namespace Editor
{
    class WindowPanel
    {
    public:
        WindowPanel(int window_flags)
            : m_WindowFlags(window_flags)
        {
        }

        virtual void OnCreate() = 0;
        virtual void OnDestroy() = 0;

        virtual void OnUpdate() = 0;

    protected:
        // Use these flags when opening an ImGui window
        int m_WindowFlags = 0;
    };
}