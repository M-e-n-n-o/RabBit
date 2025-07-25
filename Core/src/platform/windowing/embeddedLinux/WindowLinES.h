#if RB_PLATFORM_LINUX_ES

#pragma once

#include "RabBitCommon.h"
#include "graphics/Window.h"

//#include <xf86drm.h>
//#include <xf86drmMode.h>
//#include <gbm.h>
//#include <EGL/egl.h>
//#include <EGL/eglext.h>

namespace RB::Graphics::LinuxES
{
    class SwapChain;

    struct WindowArgs
    {
        uint32_t	            width;
        uint32_t	            height;
        RenderResourceFormat    format;
    };

    class WindowLinuxES : public Window
    {
    public:
        WindowLinuxES(const WindowArgs args);
        ~WindowLinuxES();

        void Update() override;

        void Present(const VsyncMode& mode) override;

        Math::Float4 GetWindowRectangle()	const override;
        uint32_t	 GetWidth()				const override;
        uint32_t	 GetHeight()			const override;
        RenderRect	 GetWindowRect()		const override;
        bool		 IsMinimized()			const override;
        bool		 IsValid()				const override;
        bool         IsSemiTransparent()    const override;

        Display* GetParentDisplay() override;

        void SetBorderless(bool borderless) override;

        bool IsSameWindow(void* window_handle) const override;
        void* GetNativeWindowHandle() const override;

        RenderResourceFormat GetBackBufferFormat() override;
        uint32_t GetCurrentBackBufferIndex() override;
        Graphics::Texture2D* GetCurrentBackBuffer() override;

    private:
        void ResizeWindow(uint32_t width, uint32_t height, int32_t x, int32_t y) override;
        void ResizeBackBuffers(uint32_t width, uint32_t height) override;
        void DestroyWindow() override;

        bool					m_IsValid;
        bool					m_IsTearingSupported;
        bool                    m_IsSemiTransparent;
    };
}
#endif