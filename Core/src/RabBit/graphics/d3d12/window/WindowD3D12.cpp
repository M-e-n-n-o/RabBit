#include "WindowD3D12.h"
#include "RabBitCommon.h"
#include "WindowD3D12.h"
#include "SwapChain.h"
#include "app/Application.h"
#include "graphics/Display.h"
#include "graphics/Renderer.h"
#include "graphics/d3d12/GraphicsDevice.h"
#include "graphics/d3d12/DeviceQueue.h"
#include "graphics/d3d12/UtilsD3D12.h"
#include "graphics/d3d12/resource/GpuResource.h"
#include "graphics/d3d12/resource/RenderResourceD3D12.h"

#include "events/WindowEvent.h"
#include "events/MouseEvent.h"
#include "events/KeyEvent.h"

using namespace RB::Events;

namespace RB::Graphics::D3D12
{
    // Window callback function
    LRESULT CALLBACK WindowCallback(HWND, UINT, WPARAM, LPARAM);

    WindowD3D12::WindowD3D12(const WindowArgs args)
        : Window(false, args.virtualScale, args.virtualAspect)
        , m_WindowHandle(nullptr)
        , m_IsValid(true)
    {
        m_IsTearingSupported = g_GraphicsDevice->IsFeatureSupported(DXGI_FEATURE_PRESENT_ALLOW_TEARING);

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
            // TODO Add the option for an HDR swapchain

            m_SwapChain = new SwapChain(
                g_GraphicsDevice->GetFactory(),
                g_GraphicsDevice->GetGraphicsQueue()->GetCommandQueue(),
                m_WindowHandle,
                width, height,
                m_IsTearingSupported,
                BACK_BUFFER_COUNT,
                args.format,
                (bool)(args.windowStyle & kWindowStyle_SemiTransparent > 0)
            );

            for (int i = 0; i < BACK_BUFFER_COUNT; ++i)
            {
                m_BackBuffers[i] = nullptr;
            }
        }

        if (args.fullscreen)
        {
            ToggleFullscreen();
        }

        ::ShowWindow(m_WindowHandle, SW_SHOW);
    }

    WindowD3D12::~WindowD3D12()
    {
        if (m_IsValid)
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Deleting window while the actual window has not yet been destroyed");
        }

        RB_LOG(LOGTAG_WINDOWING, "Destroying window");

        ::DestroyWindow(m_WindowHandle);
    }

    void WindowD3D12::Update()
    {
        MSG message = {};
        while (PeekMessage(&message, m_WindowHandle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    void WindowD3D12::Present(const VsyncMode& mode)
    {
        bool vsync_enabled = mode != VsyncMode::Off;
        UINT sync_interval = (UINT)mode;
        UINT present_flags = (m_IsTearingSupported && !vsync_enabled) ? DXGI_PRESENT_ALLOW_TEARING : 0; // DXGI_PRESENT_ALLOW_TEARING cannot be here in exclusive fullscreen!

        m_SwapChain->Present(sync_interval, present_flags);
    }

    Math::Float4 WindowD3D12::GetWindowRectangle() const
    {
        RECT window_rect;
        ::GetWindowRect(m_WindowHandle, &window_rect);

        return Math::Float4(window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, window_rect.left, window_rect.top);
    }

    uint32_t WindowD3D12::GetWidth() const
    {
        return m_SwapChain->GetWidth();
    }

    uint32_t WindowD3D12::GetHeight() const
    {
        return m_SwapChain->GetHeight();
    }

    RenderRect WindowD3D12::GetWindowRect() const
    {
        RenderRect rect = {};
        rect.width  = GetWidth();
        rect.height = GetHeight();
        rect.left   = 0;
        rect.top    = 0;
        rect.aspect = GetAspectRatio();

        return rect;
    }

    bool WindowD3D12::IsMinimized() const
    {
        return m_SwapChain->GetWidth() == 0 && m_SwapChain->GetHeight() == 0;
    }

    bool WindowD3D12::IsValid() const
    {
        return m_IsValid;
    }

    bool WindowD3D12::IsSemiTransparent() const
    {
        return m_IsSemiTransparent;
    }

    Display* WindowD3D12::GetParentDisplay()
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

    void WindowD3D12::SetBorderless(bool borderless)
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

    bool WindowD3D12::IsSameWindow(void* window_handle) const
    {
        return window_handle == m_WindowHandle;
    }

    void* WindowD3D12::GetNativeWindowHandle() const
    {
        return m_WindowHandle;
    }

    void WindowD3D12::ResizeWindow(uint32_t width, uint32_t height, int32_t x, int32_t y)
    {
        SetWindowPos(m_WindowHandle, HWND_TOP, x, y, width, height, SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED);
    }

    RenderResourceFormat WindowD3D12::GetBackBufferFormat()
    {
        return ConvertToEngineFormat(m_SwapChain->GetBackBufferFormat());
    }

    uint32_t WindowD3D12::GetCurrentBackBufferIndex()
    {
        return m_SwapChain->GetCurrentBackBufferIndex();
    }

    Graphics::Texture2D* WindowD3D12::GetCurrentBackBuffer()
    {
        if (!m_IsValid)
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Cannot retrieve backbuffer from window as it is not valid");
            return nullptr;
        }

        uint32_t index = m_SwapChain->GetCurrentBackBufferIndex();

        if (m_BackBuffers[index] == nullptr)
        {
            std::string name = "Backbuffer resource " + std::to_string(index);
            GPtr<ID3D12Resource> backbuffer = m_SwapChain->GetCurrentBackBuffer();
            m_BackBuffers[index] = Texture2D::Create(name.c_str(), new GpuResource(backbuffer, D3D12_RESOURCE_STATE_PRESENT, false), GetBackBufferFormat(), GetWidth(), GetHeight(), true, false);

            ((Texture2DD3D12*)m_BackBuffers[index])->SetRenderTargetHandle(m_SwapChain->GetCurrentDescriptorHandleCPU());
        }

        return m_BackBuffers[index];
    }

    void WindowD3D12::ResizeBackBuffers(uint32_t width, uint32_t height)
    {
        if (m_SwapChain->GetWidth() == width && m_SwapChain->GetHeight() == height)
        {
            return;
        }

        Application::GetInstance()->GetRenderer()->SyncRenderer(true);

        width = std::max(1u, width);
        height = std::max(1u, height);

        // Release backbuffer references
        for (int i = 0; i < BACK_BUFFER_COUNT; ++i)
        {
            SAFE_DELETE(m_BackBuffers[i]);
        }

        m_SwapChain->Resize(width, height);
    }

    void WindowD3D12::DestroyWindow()
    {
        // The actual window gets destroyed when deleting the Window object, this should be done by the main thread

        Application::GetInstance()->GetRenderer()->SyncRenderer(true);

        RB_LOG(LOGTAG_WINDOWING, "Scheduled destroy of window");

        m_IsValid = false;

        for (int i = 0; i < BACK_BUFFER_COUNT; ++i)
        {
            SAFE_DELETE(m_BackBuffers[i]);
        }

        delete m_SwapChain;
    }

    void WindowD3D12::RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name)
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

    void WindowD3D12::CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height, DWORD extendedStyle, DWORD style)
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
