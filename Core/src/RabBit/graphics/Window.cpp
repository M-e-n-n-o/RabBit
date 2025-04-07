#include "RabBitCommon.h"
#include "Window.h"
#include "events/WindowEvent.h"
#include "graphics/Renderer.h"
#include "graphics/Display.h"
#include "graphics/d3d12/window/WindowD3D12.h"
#include "graphics/d3d12/UtilsD3D12.h"

using namespace RB::Events;

namespace RB::Graphics
{
    Window::Window(bool is_fullscreen, float virtual_scale, float virtual_aspect)
        : m_InFocus(true)
        , m_IsFullscreen(is_fullscreen)
        , m_OriginalRect(0, 0, -1, -1)
        , m_VirtualBackBuffer(nullptr)
        , m_NewVirtualResScale(virtual_scale)
        , m_NewVirtualAspect(virtual_aspect)
    {
    }

    Window::~Window()
    {
        // Destroy window in implementation class!
    }

    Graphics::Texture2D* Window::GetVirtualBackBuffer()
    {
        if (!IsValid())
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Cannot retrieve virtual backbuffer from window as it is not valid");
            return nullptr;
        }

        if (m_VirtualBackBuffer == nullptr)
        {
            CalculateVirtualSize();
            m_VirtualBackBuffer = Texture2D::Create("Virtual backbuffer", GetBackBufferFormat(), GetVirtualWidth(), GetVirtualHeight(), true, true);
        }

        return m_VirtualBackBuffer;
    }

    void Window::Resize(uint32_t width, uint32_t height, int32_t x, int32_t y)
    {
        Math::Float2 display_res = GetParentDisplay()->GetResolution();

        if (width == 0)
        {
            width = int(display_res.x / 2.0f);
        }

        if (height == 0)
        {
            height = int(display_res.y / 2.0f);
        }

        // When x or y is -1 that coordinate will be centered to the display

        if (x == -1)
        {
            x = int32_t((display_res.x / 2.0f) - (width / 2.0f));
        }

        if (y == -1)
        {
            y = int32_t((display_res.y / 2.0f) - (height / 2.0f));
        }

        x = Math::Clamp(x, 0, int(display_res.x - (width / 2.0f)));
        y = Math::Clamp(y, 0, int(display_res.y - (height / 2.0f)));

        ResizeWindow(width, height, x, y);
    }

    bool Window::InFocus() const
    {
        return m_InFocus;
    }

    float Window::GetAspectRatio() const
    {
        return (float)GetWidth() / (float)GetHeight();
    }

    float Window::GetVirtualResolutionScale() const
    {
        return m_CurrentVirtualResScale;
    }

    float Window::GetVirtualAspectRatio() const
    {
        return (float)GetVirtualWidth() / (float)GetVirtualHeight();
    }

    RenderRect Window::GetVirtualWindowRect() const
    {
        RenderRect rect = {};
        rect.width  = m_VirtualWidth;
        rect.height = m_VirtualHeight;
        rect.left   = m_VirtualLeft;
        rect.top    = m_VirtualTop;
        rect.aspect = GetVirtualAspectRatio();

        return rect;
    }

    uint32_t Window::GetVirtualWidth() const
    {
        return m_VirtualWidth;
    }

    uint32_t Window::GetVirtualHeight() const
    {
        return m_VirtualHeight;
    }

    void Window::SetVirtualResolutionAndAspectRatio(float resolution_scale, float aspect)
    {
        m_NewVirtualResScale = resolution_scale;
        m_NewVirtualAspect = aspect;

        Math::Float4 window_rect = GetWindowRectangle();
        Resize(window_rect.x, window_rect.y, window_rect.z, window_rect.w);
    }

    void Window::ToggleFullscreen()
    {
        if (m_IsFullscreen)
        {
            SetBorderless(false);
            Resize(m_OriginalRect.x, m_OriginalRect.y, m_OriginalRect.z, m_OriginalRect.w);
        }
        else
        {
            m_OriginalRect = GetWindowRectangle();

            SetBorderless(true);

            Math::Float2 res = GetParentDisplay()->GetResolution();
            Resize(res.x, res.y, 0, 0);
        }

        m_IsFullscreen = !m_IsFullscreen;
    }

    void Window::CalculateVirtualSize()
    {
        uint32_t	width = GetWidth();
        uint32_t	height = GetHeight();
        float		aspect = (float)width / (float)height;
        float		virtual_aspect = m_NewVirtualAspect == 0.0f ? aspect : m_NewVirtualAspect;

        float		min_aspect = Math::Max(aspect, virtual_aspect);
        float		max_aspect = Math::Min(aspect, virtual_aspect);

        m_CurrentVirtualResScale = m_NewVirtualResScale;

        m_VirtualWidth  = width * m_CurrentVirtualResScale;
        m_VirtualHeight = height * m_CurrentVirtualResScale;
        m_VirtualLeft   = 0;
        m_VirtualTop    = 0;

        float epsilon = 0.001f;
        if ((min_aspect - aspect) > epsilon)
        {
            float corrected_height = (float)m_VirtualWidth / min_aspect;
            float over_size        = m_VirtualHeight - corrected_height;

            m_VirtualTop    += Math::Max(0u, (uint32_t)(over_size * 0.5f));
            m_VirtualHeight -= (uint32_t)(Math::Floor(over_size));
        }
        else if ((aspect - max_aspect) > epsilon)
        {
            float corrected_width = (float)m_VirtualHeight * max_aspect;
            float over_size       = m_VirtualWidth - corrected_width;

            m_VirtualLeft  += Math::Max(0u, (uint32_t)(over_size * 0.5f));
            m_VirtualWidth -= (uint32_t)(Math::Floor(over_size));
        }

        m_VirtualLeft /= m_CurrentVirtualResScale;
        m_VirtualTop /= m_CurrentVirtualResScale;
    }

    void Window::ProcessEvent(WindowEvent& window_event)
    {
        // Only handle window events of this window
        if (!IsSameWindow(window_event.GetWindowHandle()))
        {
            return;
        }

        switch (window_event.GetEventType())
        {
        case EventType::WindowCreated:
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "We should not receive this Window Created event");
        }
        break;

        case EventType::WindowMoved:
        {
            RB_LOG(LOGTAG_WINDOWING, "Moved");
        }
        break;

        case EventType::WindowFullscreenToggle:
        {
            ToggleFullscreen();
        }
        break;

        case EventType::WindowResize:
        {
            WindowResizeEvent* resize_event = static_cast<WindowResizeEvent*>(&window_event);

            uint32_t width = resize_event->GetWidth();
            uint32_t height = resize_event->GetHeight();

            RB_LOG(LOGTAG_WINDOWING, "Resizing window to (%d x %d)", width, height);

            if (resize_event->IsWindowAlreadyResized())
            {
                // Do the swapchain resize
                ResizeBackBuffers(width, height);

                // Delete the virtual backbuffer
                SAFE_DELETE(m_VirtualBackBuffer);
            }
            else
            {
                // First resize the actual window, which will create another resize event which will resize the swapchain
                Resize(width, height, -1, -1);
            }
        }
        break;

        case EventType::WindowFocus:
        {
            m_InFocus = true;
        }
        break;

        case EventType::WindowLostFocus:
        {
            m_InFocus = false;
        }
        break;

        case EventType::WindowCloseRequest:
        {
            DestroyWindow();
            SAFE_DELETE(m_VirtualBackBuffer);
        }
        break;

        case EventType::WindowClose:
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Window::ProcessEvent should not get a WindowClose event!");
        }
        break;

        default:
            break;
        }
    }

    Window* Window::Create(const char* window_name, Display* display, uint32_t window_style, float virtual_scale, float virtual_aspect)
    {
        switch (Renderer::GetAPI())
        {
        case RenderAPI::D3D12:
        {
            D3D12::WindowArgs args = {};
            args.className      = L"RabBit WindowClass";
            args.instance       = GetModuleHandle(nullptr);
            args.fullscreen     = true;
            args.width          = display->GetResolution().x;
            args.height         = display->GetResolution().y;
            args.virtualScale   = virtual_scale;
            args.virtualAspect  = virtual_aspect;
            args.windowStyle    = window_style;
            args.windowName     = window_name;
            args.format         = D3D12::ConvertToDXGIFormat(RenderResourceFormat::R16G16B16A16_FLOAT); // TODO Do this based on the display

            return new D3D12::WindowD3D12(args);
        }
        default:
            RB_LOG_CRITICAL(LOGTAG_WINDOWING, "Did not yet implement the window class for the set graphics API");
            break;
        }

        return nullptr;
    }

    Window* Window::Create(const char* window_name, uint32_t window_width, uint32_t window_height, uint32_t window_style, RenderResourceFormat window_format, float virtual_scale, float virtual_aspect)
    {
        switch (Renderer::GetAPI())
        {
        case RenderAPI::D3D12:
        {
            D3D12::WindowArgs args = {};
            args.className      = L"RabBit WindowClass";
            args.instance       = GetModuleHandle(nullptr);
            args.fullscreen     = false;
            args.width          = window_width;
            args.height         = window_height;
            args.virtualScale   = virtual_scale;
            args.virtualAspect  = virtual_aspect;
            args.windowStyle    = window_style;
            args.windowName     = window_name;
            args.format         = D3D12::ConvertToDXGIFormat(window_format);

            return new D3D12::WindowD3D12(args);
        }
        default:
            RB_LOG_CRITICAL(LOGTAG_WINDOWING, "Did not yet implement the window class for the set graphics API");
            break;
        }

        return nullptr;
    }
}