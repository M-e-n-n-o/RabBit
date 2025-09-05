#if RB_PLATFORM_WINDOWS

#include "RabBitCommon.h"
#include "WindowWin.h"
#include "app/Application.h"
#include "graphics/Display.h"
#include "graphics/Renderer.h"

#include "events/WindowEvent.h"
#include "events/MouseEvent.h"
#include "events/KeyEvent.h"

#if RB_GRAPHICS_API_D3D12
#include "platform/graphics/d3d12/SwapChainD3D12.h"
#endif

#if RB_GRAPHICS_API_VULKAN
#include "platform/graphics/vulkan/SwapChainVK.h"
#endif

using namespace RB::Events;

namespace RB::Graphics::Windows
{
    // Window callback function
    LRESULT CALLBACK WindowCallback(HWND, UINT, WPARAM, LPARAM);

    WindowWin::WindowWin(const WindowArgs& args)
        : Window(false, args.virtualScale, args.virtualAspect)
        , m_WindowHandle(nullptr)
        , m_IsValid(true)
        , m_BackBufferFormat(args.format)
    {
        RegisterWindowCLass(args.instance, args.className);

        DWORD style = WS_OVERLAPPEDWINDOW;
        DWORD extended_style = NULL;
        
        m_IsSemiTransparent = false;

        if (args.windowStyle & kWindowStyle_SemiTransparent)
        {
            extended_style = WS_EX_NOREDIRECTIONBITMAP;
            m_IsSemiTransparent = true;
        }

        uint32_t width = args.width;
        uint32_t height = args.height;

        // Create window
        {
            wchar_t* wchar_name = new wchar_t[strlen(args.windowName) + 1];
            CharToWchar(args.windowName, wchar_name);

            CreateWindow(args.instance, args.className, wchar_name, width, height, extended_style, style);

            delete[] wchar_name;
        }

        // Create swapchain
        {
            bool transparency_support = (args.windowStyle & kWindowStyle_SemiTransparent) > 0;

            // TODO Add the option for an HDR swapchain

            switch (Renderer::GetAPI())
            {
#if RB_GRAPHICS_API_D3D12
            case RenderAPI::D3D12:
            {
                m_SwapChain = new D3D12::SwapChainD3D12(
                    m_WindowHandle,
                    width, height,
                    args.vsync,
                    BACK_BUFFER_COUNT,
                    args.format,
                    transparency_support
                );
            }
            break;
#endif
#if RB_GRAPHICS_API_VULKAN
            case RenderAPI::Vulkan:
            {
                m_SwapChain = new VK::SwapChainVK(
                    m_WindowHandle, args.instance,
                    width, height,
                    args.vsync,
                    BACK_BUFFER_COUNT,
                    args.format,
                    transparency_support
                );
            }
            break;
#endif

            default:
                RB_LOG_ERROR(LOGTAG_WINDOWING, "Did not yet implement a swapchain class for this graphics API");
                break;
            }

        }

        if (args.fullscreen)
        {
            ToggleFullscreen();
        }

        ::ShowWindow(m_WindowHandle, SW_SHOW);
    }

    WindowWin::~WindowWin()
    {
        if (m_IsValid)
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Deleting window while the actual window has not yet been destroyed");
        }

        RB_LOG(LOGTAG_WINDOWING, "Destroying window");

        ::DestroyWindow(m_WindowHandle);
    }

    void WindowWin::Update()
    {
        MSG message = {};
        while (PeekMessage(&message, m_WindowHandle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    void WindowWin::Present()
    {
        m_SwapChain->Present();
    }

    Math::Float4 WindowWin::GetNativeWindowRectangle() const
    {
        RECT window_rect;
        ::GetWindowRect(m_WindowHandle, &window_rect);

        return Math::Float4(window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, window_rect.left, window_rect.top);
    }

    uint32_t WindowWin::GetWidth() const
    {
        return m_SwapChain->GetWidth();
    }

    uint32_t WindowWin::GetHeight() const
    {
        return m_SwapChain->GetHeight();
    }

    RenderRect WindowWin::GetWindowRect() const
    {
        RenderRect rect = {};
        rect.width  = GetWidth();
        rect.height = GetHeight();
        rect.left   = 0;
        rect.top    = 0;
        rect.aspect = GetAspectRatio();

        return rect;
    }

    bool WindowWin::IsMinimized() const
    {
        return m_SwapChain->GetWidth() == 0 && m_SwapChain->GetHeight() == 0;
    }

    bool WindowWin::IsValid() const
    {
        return m_IsValid;
    }

    bool WindowWin::IsSemiTransparent() const
    {
        return m_IsSemiTransparent;
    }

    Display* WindowWin::GetParentDisplay()
    {
        List<Display*> displays = Application::GetInstance()->GetDisplays();

        HMONITOR parent = MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTONEAREST);

        for (int i = 0; i < displays.size(); ++i)
        {
            if (displays[i]->GetNativeHandle() == parent)
            {
                return displays[i];
            }
        }

        RB_LOG_WARN(LOGTAG_WINDOWING, "Could not find parent display");

        return nullptr;
    }

    void WindowWin::SetBorderless(bool borderless)
    {
        if (borderless)
        {
            SetWindowLongPtr(m_WindowHandle, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        }
        else
        {
            SetWindowLongPtr(m_WindowHandle, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
        }
    }

    bool WindowWin::IsSameWindow(void* window_handle) const
    {
        return window_handle == m_WindowHandle;
    }

    void* WindowWin::GetNativeWindowHandle() const
    {
        return m_WindowHandle;
    }

    void WindowWin::ResizeWindow(uint32_t width, uint32_t height, int32_t x, int32_t y)
    {
        SetWindowPos(m_WindowHandle, HWND_TOP, x, y, width, height, SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED);
    }

    RenderResourceFormat WindowWin::GetBackBufferFormat()
    {
        return m_BackBufferFormat;
    }

    uint32_t WindowWin::GetCurrentBackBufferIndex()
    {
        return m_SwapChain->GetCurrentBackBufferIndex();
    }

    Graphics::Texture2D* WindowWin::GetCurrentBackBuffer()
    {
        if (!m_IsValid)
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Cannot retrieve backbuffer from window as it is not valid");
            return nullptr;
        }

        return m_SwapChain->GetCurrentBackBuffer();
    }

    void WindowWin::ResizeBackBuffers(uint32_t width, uint32_t height)
    {
        if (m_SwapChain->GetWidth() == width && m_SwapChain->GetHeight() == height)
        {
            return;
        }

        Application::GetInstance()->GetRenderer()->SyncRenderer(true);

        width = std::max(1u, width);
        height = std::max(1u, height);

        m_SwapChain->Resize(width, height);
    }

    void WindowWin::DestroyWindow()
    {
        // The actual window gets destroyed when deleting the Window object, this should be done by the main thread

        Application::GetInstance()->GetRenderer()->SyncRenderer(true);

        RB_LOG(LOGTAG_WINDOWING, "Scheduled destroy of window");

        m_IsValid = false;

        delete m_SwapChain;
    }

    void WindowWin::RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name)
    {
        WNDCLASSEXW window_class = {};

        window_class.cbSize         = sizeof(WNDCLASSEXW);
        window_class.style          = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc    = &WindowCallback;
        window_class.cbClsExtra     = 0;
        window_class.cbWndExtra     = 0;
        window_class.hInstance      = instance;
        window_class.hIcon          = ::LoadIcon(instance, "0");			// MAKEINTRESOURCE(APP_ICON)
        window_class.hCursor        = ::LoadCursor(NULL, IDC_ARROW);
        window_class.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        window_class.lpszMenuName   = NULL;
        window_class.lpszClassName  = class_name;
        window_class.hIconSm        = ::LoadIcon(instance, "0");			// MAKEINTRESOURCE(APP_ICON)

        HRESULT result = ::RegisterClassExW(&window_class);
        RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, SUCCEEDED(result), "Failed to register window");
    }

    void WindowWin::CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height, DWORD extendedStyle, DWORD style)
    {
        int screen_width = ::GetSystemMetrics(SM_CXSCREEN);
        int screen_height = ::GetSystemMetrics(SM_CYSCREEN);

        RECT window_rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        ::AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

        int window_width = window_rect.right - window_rect.left;
        int window_height = window_rect.bottom - window_rect.top;

        // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
        int window_x = std::max<int>(0, (screen_width - window_width) / 2);
        int window_y = std::max<int>(0, (screen_height - window_height) / 2);

        m_WindowHandle = ::CreateWindowExW(
            extendedStyle,
            class_name,
            window_title,
            style,
            window_x,
            window_y,
            window_width,
            window_height,
            NULL,
            NULL,
            instance,
            nullptr
        );

        RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, m_WindowHandle, "Failed to create window");
    }

    LRESULT CALLBACK WindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            KeyPressedEvent e(static_cast<KeyCode>(wParam), false);
            g_EventManager->InsertEvent(e);
        }
        break;
        case WM_SYSCHAR:
            break;
        case WM_SIZE:
        {
            RECT client_rect = {};
            ::GetClientRect(hwnd, &client_rect);

            uint32_t width = client_rect.right - client_rect.left;
            uint32_t height = client_rect.bottom - client_rect.top;

            WindowResizeEvent e(hwnd, width, height, true);
            g_EventManager->InsertEvent(e);
        }
        break;
        case WM_SETFOCUS:
        {
            WindowOnFocusEvent e(hwnd);
            g_EventManager->InsertEvent(e);
        }
        break;
        case WM_KILLFOCUS:
        {
            WindowLostFocusEvent e(hwnd);
            g_EventManager->InsertEvent(e);
        }
        break;
        case WM_CLOSE:
        {
            WindowCloseRequestEvent e(hwnd);
            g_EventManager->InsertEvent(e);
        }
        break;
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
        default:
        {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
        }

        return 0;
    }
}
#endif