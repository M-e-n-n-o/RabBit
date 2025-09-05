#if RB_PLATFORM_LINUX_ES

#include "RabBitCommon.h"
#include "WindowLinES.h"
#include "app/Application.h"
#include "graphics/Renderer.h"

#if RB_GRAPHICS_API_VULKAN
#include "platform/graphics/vulkan/SwapChainVK.h"
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> 

namespace RB::Graphics::LinuxES
{
    WindowLinuxES::WindowLinuxES(const WindowArgs& args)
        : Window(true, args.virtualScale, args.virtualAspect)
        , m_IsValid(false)
        , m_BackBufferFormat(args.format)
    {
        m_TTY = open("/dev/tty1", O_RDWR | O_NOCTTY);
        RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, m_TTY >= 0, "Could not open TTY");
        ioctl(m_TTY, KDSETMODE, KD_GRAPHICS); // switch to graphics mode

        //RB_ASSERT(LOGTAG_WINDOWING, args.format == RenderResourceFormat::B8G8R8A8_UNORM, "Currently only the B8G8R8A8_UNORM format is supported on a embedded linux window");
        //
        //InitializeDRM(args);
        //
        //RB_LOG(LOGTAG_WINDOWING, "Embedded window initialized: connector %u, crtc %u, mode %ux%u", m_DrmConnectorId, m_CrtcId, m_DrmMode.hdisplay, m_DrmMode.vdisplay);

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

    //void WindowLinuxES::InitializeDRM(const WindowArgs& args)
    //{
    //    const char* drm_device_name = args.drmDeviceName ? args.drmDeviceName : "/dev/dri/card0";
    //    m_DrmFileDescriptor = open(drm_device_name, O_RDWR | O_CLOEXEC);
    //    RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, m_DrmFileDescriptor >= 0, "Could not open DRM device");

    //    m_DrmResources = drmModeGetResources(m_DrmFileDescriptor);
    //    RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, m_DrmResources != nullptr, "Could not get the DRM resources");

    //    // Choose first connected connector with a mode
    //    drmModeConnector* chosen_conn = nullptr;
    //    for (int i = 0; i < m_DrmResources->count_connectors; ++i)
    //    {
    //        drmModeConnector* conn = drmModeGetConnector(m_DrmFileDescriptor, m_DrmResources->connectors[i]);

    //        if (!conn)
    //        {
    //            continue;
    //        }

    //        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
    //        {
    //            chosen_conn = conn;
    //            break;
    //        }

    //        drmModeFreeConnector(conn);
    //    }
    //    RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, chosen_conn != nullptr, "No connected DRM connector found");
    //    m_DrmConnector = chosen_conn;
    //    m_DrmConnectorId = chosen_conn->connector_id;

    //    // Pick preferred mode
    //    int preferred = -1;
    //    for (int i = 0; i < m_DrmConnector->count_modes; ++i)
    //    {
    //        if (m_DrmConnector->modes[i].type & DRM_MODE_TYPE_PREFERRED)
    //        { 
    //            preferred = i; break; 
    //        }
    //    }

    //    m_DrmMode = m_DrmConnector->modes[Math::Max(0, preferred)];

    //    RB_ASSERT(LOGTAG_WINDOWING, m_DrmMode.hdisplay == args.width && m_DrmMode.vdisplay == args.height, "The specified display size does not match the output display's size");

    //    // Choose a CRTC
    //    drmModeEncoder* encoder = nullptr;
    //    if (m_DrmConnector->encoder_id)
    //    {
    //        encoder = drmModeGetEncoder(m_DrmFileDescriptor, m_DrmConnector->encoder_id);
    //    }

    //    if (!encoder)
    //    {
    //        // Fallback: pick first encoder for connector
    //        for (int i = 0; i < m_DrmConnector->count_encoders && !encoder; ++i)
    //        {
    //            encoder = drmModeGetEncoder(m_DrmFileDescriptor, m_DrmConnector ->encoders[i]);
    //        }
    //    }
    //    RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, encoder != nullptr, "Failed to get encoder for connector");

    //    // Find a compatible CRTC
    //    bool found_crtc = false;
    //    if (encoder->crtc_id)
    //    {
    //        m_CrtcId = encoder->crtc_id;
    //        found_crtc = true;
    //    }
    //    else
    //    {
    //        // Fallback: choose first possible crtc
    //        for (int i = 0; i < m_DrmResources->count_crtcs; ++i)
    //        {
    //            if (encoder->possible_crtcs & (1 << i))
    //            {
    //                m_CrtcId = m_DrmResources->crtcs[i];
    //                found_crtc = true;
    //                break;
    //            }
    //        }
    //    }
    //    drmModeFreeEncoder(encoder);
    //    RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, found_crtc, "Failed to pick a CRTC for the connector");

    //    // Save original CRTC to restore on destroy
    //    m_OriginalCrtc = drmModeGetCrtc(m_DrmFileDescriptor, m_CrtcId);
    //}

    //void WindowLinuxES::InitializeGBM(const WindowArgs& args)
    //{
    //    // Create GBM device
    //    m_GbmDevice = gbm_create_device(m_DrmFileDescriptor);
    //    RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, m_GbmDevice != nullptr, "Failed to create GBM device");

    //    // Create GBM surface sized to the selected mode
    //    const uint32_t format = GBM_FORMAT_XBGR8888;
    //    const uint32_t flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
    //    m_GbmSurface = gbm_surface_create(m_GbmDevice, m_DrmMode.hdisplay, m_DrmMode.vdisplay, format, flags);
    //    RB_ASSERT_FATAL_RELEASE(LOGTAG_WINDOWING, m_GbmSurface != nullptr, "Failed to create GBM surface");

    //    // TODO set the vulkan backbuffer images as the framebuffers in drm (drmModeAddFB2)
    //    //          - to do this I think I need to 
    //    // TODO set crtc to use these new framebuffers.
    //    // 
    //    // I am actually not yet sure if I need GBM at all here, maybe I can bypass it?
    //    // Also not completely sure yet if I can just use vulkan's vkQueuePresentKHR for presenting or that I need something more manual
    //    //
    //    // Maybe I actually don't need GBM or DRM at all and just use VK_KHR_display??
    //}

    WindowLinuxES::~WindowLinuxES()
    {
        if (m_IsValid)
        {
            RB_LOG_ERROR(LOGTAG_WINDOWING, "Deleting window while the actual window has not yet been destroyed");
        }

        RB_LOG(LOGTAG_WINDOWING, "Destroying window");

        ioctl(m_TTY, KDSETMODE, KD_TEXT);
        close(m_TTY);

        //if (m_GbmSurface)
        //{
        //    gbm_surface_destroy(m_GbmSurface);
        //}

        //if (m_GbmDevice)
        //{
        //    gbm_device_destroy(m_GbmDevice);
        //}

        //if (m_OriginalCrtc)
        //{
        //    // Restore original mode/CRTC if we changed it during present (safe to call even if we didn't modeset)
        //    drmModeSetCrtc(m_DrmFileDescriptor, m_OriginalCrtc->crtc_id, m_OriginalCrtc->buffer_id, m_OriginalCrtc->x, m_OriginalCrtc->y, &m_DrmConnectorId, 1, &m_OriginalCrtc->mode);
        //    drmModeFreeCrtc(m_OriginalCrtc);
        //}

        //if (m_DrmConnector)
        //{
        //    drmModeFreeConnector(m_DrmConnector);
        //}

        //if (m_DrmResources)
        //{
        //    drmModeFreeResources(m_DrmResources);
        //}

        //if (m_DrmFileDescriptor >= 0)
        //{
        //    close(m_DrmFileDescriptor);
        //}
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
        return Math::Float4(GetWidth(), GetHeight(), 0, 0);
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

        RB_LOG(LOGTAG_WINDOWING, "Destroying swapchain");

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
        return true; //m_DrmConnector == (drmModeConnector**)window_handle;
    }

    void* WindowLinuxES::GetNativeWindowHandle() const
    {
        RB_LOG_WARN(LOGTAG_WINDOWING, "TODO");
        return nullptr; //m_DrmConnector;
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