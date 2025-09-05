#if RB_PLATFORM_LINUX_ES

#include "RabBitCommon.h"
#include "WindowLinES.h"
#include "app/Application.h"
#include "graphics/Renderer.h"

#if RB_GRAPHICS_API_VULKAN
#include "platform/graphics/vulkan/SwapChainVK.h"
#endif

namespace RB::Graphics::LinuxES
{
    WindowLinuxES::WindowLinuxES(const WindowArgs& args)
        : Window(true, args.virtualScale, args.virtualAspect)
        , m_IsValid(false)
        , m_BackBufferFormat(args.format)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_VULKAN
        case RenderAPI::Vulkan:
        {
            m_SwapChain = new VK::SwapChainVK(
                args.display,
                args.width,
                args.height,
                args.vsync,
                BACK_BUFFER_COUNT,
                args.format
            );
        }
        break;
#endif
        default:
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Did not yet implement a swapchain class for this graphics API");
            break;
        }


        m_IsValid = true;
    }

    WindowLinuxES::~WindowLinuxES()
    {
        
    }

    void WindowLinuxES::Update()
    {
        RB_LOG_WARN(LOGTAG_WINDOWING, "TODO");
    }

    void WindowLinuxES::Present()
    {
        m_SwapChain->Present();
    }

    Math::Float4 WindowLinuxES::GetNativeWindowRectangle() const
    {
        return Math::Float4(GetWidth(), getHeight(), 0, 0);
    }

    uint32_t WindowLinuxES::GetWidth() const
    {
        return m_SwapChain->GetWidth();
    }

    uint32_t WindowLinuxES::GetHeight() const
    {
        return m_SwapChain->GetHeight();
    }

    RenderRect WindowLinuxES::GetWindowRect() const
    {
        RenderRect rect = {};
        rect.width  = GetWidth();
        rect.height = GetHeight();
        rect.left   = 0;
        rect.top    = 0;
        rect.aspect = GetAspectRatio();

        return rect;
    }

    bool WindowLinuxES::IsMinimized() const
    {
        // Cannot minimize a native window
        return false;
    }

    bool WindowLinuxES::IsValid() const
    {
        return m_IsValid;
    }

    bool WindowLinuxES::IsSemiTransparent() const
    {
        // A native window cannot be transparent
        return false;
    }

    Display* WindowLinuxES::GetParentDisplay()
    {
        RB_LOG_WARN(LOGTAG_WINDOWING, "TODO");
        return nullptr;
    }
    
    void WindowLinuxES::DestroyWindow()
    {
        // Do most of the actual destroy's in the destructor as that is called from the main thread, this is not
        Application::GetInstance()->GetRenderer()->SyncRenderer(true);

        RB_LOG(LOGTAG_WINDOWING, "Destroying window");

        delete m_SwapChain;
        m_IsValid = false;
    }

    void WindowLinuxES::SetBorderless(bool borderless)
    {
        RB_LOG_ERROR(LOGTAG_WINDOWING, "Setting border mode is not supported on this platform");
    }

    bool WindowLinuxES::IsSameWindow(void* window_handle) const
    {
        RB_LOG_WARN(LOGTAG_WINDOWING, "TODO");
        return false;
    }

    void* WindowLinuxES::GetNativeWindowHandle() const
    {
        RB_LOG_WARN(LOGTAG_WINDOWING, "TODO");
        return nullptr;
    }

    RenderResourceFormat WindowLinuxES::GetBackBufferFormat()
    {
        return m_BackBufferFormat;
    }

    uint32_t WindowLinuxES::GetCurrentBackBufferIndex()
    {
        return m_SwapChain->GetCurrentBackBufferIndex();
    }

    Graphics::Texture2D* WindowLinuxES::GetCurrentBackBuffer()
    {
        if (!m_IsValid)
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Cannot retrieve backbuffer from window as it is not valid");
            return nullptr;
        }

        return m_SwapChain->GetCurrentBackBuffer();
    }

    void WindowLinuxES::ResizeWindow(uint32_t width, uint32_t height, int32_t x, int32_t y)
    {
        RB_LOG_ERROR(LOGTAG_WINDOWING, "Resizing is not supported on this platform");
    }

    void WindowLinuxES::ResizeBackBuffers(uint32_t width, uint32_t height)
    {
        RB_LOG_ERROR(LOGTAG_WINDOWING, "Resizing is not supported on this platform");
    }
}
#endif