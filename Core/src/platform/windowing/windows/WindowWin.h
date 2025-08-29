#if RB_PLATFORM_WINDOWS

#pragma once

#include "RabBitCommon.h"
#include "graphics/Window.h"
#include "platform/windowing/SwapChain.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

namespace RB::Graphics::Windows
{
    struct WindowArgs
    {
        HINSTANCE               instance;
        wchar_t*                className;
        const char*             windowName;
        bool                    fullscreen;
        uint32_t                width;
        uint32_t                height;
        bool                    vsync;
        float                   virtualScale;
        float                   virtualAspect;
        uint32_t                windowStyle;
        RenderResourceFormat    format;
    };

    class WindowWin : public Window
    {
    public:
        WindowWin(const WindowArgs& args);
        ~WindowWin();

        void Update() override;

        void Present() override;

        Math::Float4 GetWindowRectangle()   const override;
        uint32_t     GetWidth()             const override;
        uint32_t     GetHeight()            const override;
        RenderRect   GetWindowRect()        const override;
        bool         IsMinimized()          const override;
        bool         IsValid()              const override;
        bool         IsSemiTransparent()    const override;

        Display* GetParentDisplay() override;

        void SetBorderless(bool borderless) override;

        bool IsSameWindow(void* window_handle) const override;
        void* GetNativeWindowHandle() const override;

        RenderResourceFormat GetBackBufferFormat() override;
        uint32_t GetCurrentBackBufferIndex() override;
        Graphics::Texture2D* GetCurrentBackBuffer() override;

        HWND GetHandle() const { return m_WindowHandle; }

    private:
        void ResizeWindow(uint32_t width, uint32_t height, int32_t x, int32_t y) override;
        void ResizeBackBuffers(uint32_t width, uint32_t height) override;
        void DestroyWindow() override;

        void RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name);

        void CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height, DWORD extendedStyle, DWORD style);

        HWND                    m_WindowHandle;
        SwapChain*              m_SwapChain;
        bool                    m_IsValid;
        bool                    m_IsSemiTransparent;
        RenderResourceFormat    m_BackBufferFormat;
    };
}
#endif