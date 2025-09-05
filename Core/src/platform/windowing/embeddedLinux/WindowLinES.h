#if RB_PLATFORM_LINUX_ES

#pragma once

#include "RabBitCommon.h"
#include "graphics/Window.h"
#include "graphics/Display.h"
#include "platform/windowing/SwapChain.h"

//#include <xf86drm.h>
//#include <xf86drmMode.h>
//#include <gbm.h>

namespace RB::Graphics::LinuxES
{
    struct WindowArgs
    {
        //const char*             drmDeviceName
        Display*                display;    
        uint32_t                width;
        uint32_t                height;
        bool                    vsync;
        float                   virtualScale;
        float                   virtualAspect;
        RenderResourceFormat    format;
    };

    class WindowLinuxES : public Window
    {
    public:
        WindowLinuxES(const WindowArgs& args);
        ~WindowLinuxES();

        void Update() override;

        void Present() override;

        Math::Float4 GetNativeWindowRectangle() const override;
        uint32_t     GetWidth()                 const override;
        uint32_t     GetHeight()                const override;
        RenderRect   GetWindowRect()            const override;
        bool         IsMinimized()              const override;
        bool         IsValid()                  const override;
        bool         IsSemiTransparent()        const override;

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

        //int                     m_DrmFileDescriptor;
        //drmModeRes*             m_DrmResources;
        //uint32_t                m_DrmConnectorId;
        //drmModeConnector*       m_DrmConnector;
        //drmModeModeInfo         m_DrmMode;
        //uint32_t                m_CrtcId;
        //drmModeCrtc*            m_OriginalCrtc;
        //
        //gbm_device*             m_GbmDevice;
        //gbm_surface*            m_GbmSurface;

        int                     m_TTY;

        SwapChain*              m_SwapChain;
        bool                    m_IsValid;
        RenderResourceFormat    m_BackBufferFormat;
    };
}
#endif