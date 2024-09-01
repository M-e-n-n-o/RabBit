#pragma once

#include "input/events/WindowEvent.h"
#include "graphics/RenderResource.h"
#include "math/Vector.h"

namespace RB::Graphics
{
    enum WindowStyle : uint32_t
    {
        kWindowStyle_Default            = (0),
        kWindowStyle_SemiTransparent    = (1 << 0)
    };

    enum class VsyncMode : uint8_t
    {
        Off     = 0,
        On      = 1,
        Half    = 2,
        Quarter = 3,
        Eighth  = 4
    };

    struct RenderRect
    {
        uint32_t left;
        uint32_t top;
        uint32_t width;
        uint32_t height;
        float	 aspect;
    };

    class Display;

    static const uint32_t BACK_BUFFER_COUNT = 2u;

    class Window
    {
    public:
        virtual ~Window();

        virtual void			Update() = 0;

        virtual void			Present(const VsyncMode& mode) = 0;

        // Returns width, height, x pos, and y pos of entire window
        virtual Math::Float4	GetWindowRectangle()	const = 0;

        float			        GetAspectRatio()		const;
        virtual RenderRect		GetWindowRect()			const = 0;
        virtual uint32_t		GetWidth()				const = 0;
        virtual uint32_t		GetHeight()				const = 0;

        float			        GetVirtualResolutionScale() const;
        float			        GetVirtualAspectRatio() const;
        RenderRect		        GetVirtualWindowRect()	const;
        uint32_t		        GetVirtualWidth()		const;
        uint32_t		        GetVirtualHeight()		const;
        void			        SetVirtualResolutionAndAspectRatio(float resolution_scale, float aspect);

        virtual bool			IsMinimized()			const = 0;
        virtual bool			IsValid()				const = 0;

        bool			        InFocus()				const;

        virtual Display*        GetParentDisplay() = 0;

        virtual void			SetBorderless(bool borderless) = 0;

        virtual bool			IsSameWindow(void* window_handle)	const = 0;
        virtual void*           GetNativeWindowHandle()				const = 0;

        virtual RenderResourceFormat	GetBackBufferFormat() = 0;
        virtual uint32_t				GetCurrentBackBufferIndex() = 0;
        virtual Graphics::Texture2D*    GetCurrentBackBuffer() = 0;
        Graphics::Texture2D*            GetVirtualBackBuffer();

        void		   Resize(uint32_t width, uint32_t height, int32_t x = -1, int32_t y = -1);

        void		   ProcessEvent(Input::Events::WindowEvent& event);

        static Window* Create(const char* window_name, Display* display, uint32_t window_style, float virtual_resolution_scale = 1, float virtual_aspect = 0);
        static Window* Create(const char* window_name, uint32_t window_width, uint32_t window_height, uint32_t window_style, float virtual_resolution_scale = 1, float virtual_aspect = 0);

    protected:
        Window(bool is_fullscreen, float virtual_scale, float virtual_aspect);

        // These should be called via events
        void ToggleFullscreen();

    private:
        virtual void DestroyWindow() = 0;
        virtual void ResizeWindow(uint32_t width, uint32_t height, int32_t x, int32_t y) = 0;
        virtual void ResizeBackBuffers(uint32_t width, uint32_t height) = 0;

        void CalculateVirtualSize();

        bool					m_InFocus;

        bool					m_IsFullscreen;
        Math::Float4			m_OriginalRect;

        uint32_t				m_VirtualWidth;
        uint32_t				m_VirtualHeight;
        uint32_t				m_VirtualTop;
        uint32_t				m_VirtualLeft;
        float					m_CurrentVirtualResScale;
        float					m_NewVirtualResScale;
        float					m_NewVirtualAspect;
        Graphics::Texture2D*    m_VirtualBackBuffer;
    };
}