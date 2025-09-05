#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "SwapChainVK.h"
#include "GraphicsDevice.h"
#include "DeviceQueue.h"
#include "GpuResource.h"
#include "RenderResourceVK.h"
#include "UtilsVK.h"

namespace RB::Graphics::VK
{
#if RB_PLATFORM_WINDOWS
    SwapChainVK::SwapChainVK(HWND window_handle, HINSTANCE h_instance, uint32_t width, uint32_t height, bool vsync, uint32_t buffer_count, RenderResourceFormat format, bool transparency_support)
    {
        VkWin32SurfaceCreateInfoKHR info = {};
        info.sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        info.hinstance  = h_instance;
        info.hwnd       = window_handle;

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateWin32SurfaceKHR(g_GraphicsDevice->GetInstance(), &info, nullptr, &m_Surface), "Failed to create Win32 surface");

        Init(width, height, vsync, buffer_count, format, transparency_support);
    }
#elif RB_PLATFORM_LINUX_ES
    SwapChainVK::SwapChainVK(Display* display, uint32_t width, uint32_t height, bool vsync, uint32_t buffer_count, RenderResourceFormat format)
    {
        VkDisplayKHR native_display = *(VkDisplayKHR*)display->GetNativeHandle();

        // Find a plane compatible with the display
        uint32_t plane_count = 0;
        vkGetPhysicalDeviceDisplayPlanePropertiesKHR(g_GraphicsDevice->GetPhysicalDevice(), &plane_count, nullptr);

        uint32_t plane_index = UINT32_MAX;
        for (uint32_t i = 0; i < plane_count; i++)
        {
            uint32_t mode_count = 0;
            vkGetDisplayPlaneSupportedDisplaysKHR(g_GraphicsDevice->GetPhysicalDevice(), i, &mode_count, nullptr);
            std::vector<VkDisplayKHR> supported_displays(mode_count);
            vkGetDisplayPlaneSupportedDisplaysKHR(g_GraphicsDevice->GetPhysicalDevice(), i, &mode_count, supported_displays.data());

            if (std::find(supported_displays.begin(), supported_displays.end(), native_display) != supported_displays.end())
            {
                plane_index = i;
                break;
            }
        }
        RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, plane_index != UINT32_MAX, "No compatible plane found for the display!");

        // Pick a display mode (first mode)
        uint32_t mode_count = 0;
        vkGetDisplayModePropertiesKHR(g_GraphicsDevice->GetPhysicalDevice(), native_display, &mode_count, nullptr);
        RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, mode_count > 0, "No display modes available!");
        std::vector<VkDisplayModePropertiesKHR> modes(mode_count);
        vkGetDisplayModePropertiesKHR(g_GraphicsDevice->GetPhysicalDevice(), native_display, &mode_count, modes.data());

        uint32_t mode_index = UINT32_MAX;
        for (uint32_t i = 0; i < mode_count; i++)
        {
            if (modes[i].parameters.visibleRegion.width == width &&
                modes[i].parameters.visibleRegion.height == height)
            {
                mode_index = i;
                break;
            }
        }

        RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, mode_index != UINT32_MAX, "No compatible display mode found for the display!");

        // Create the plane surface
        VkDisplaySurfaceCreateInfoKHR surface_info = {};
        surface_info.sType           = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
        surface_info.displayMode     = modes[mode_index].displayMode;
        surface_info.planeIndex      = plane_index;
        surface_info.planeStackIndex = 0;
        surface_info.transform       = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        surface_info.globalAlpha     = 1.0f;
        surface_info.alphaMode       = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
        surface_info.imageExtent     = modes[mode_index].parameters.visibleRegion;

        RB_ASSERT_FATAL_RELEASE_VK(
            vkCreateDisplayPlaneSurfaceKHR(g_GraphicsDevice->GetInstance(), &surface_info, nullptr, &m_Surface),
            "Failed to create Vulkan display plane surface"
        );

        Init(width, height, vsync, buffer_count, format, false);
    }
#endif

    void SwapChainVK::Init(uint32_t width, uint32_t height, bool vsync, uint32_t buffer_count, RenderResourceFormat format, bool transparency_support)
    {
        // If this goes off, then we need to create a separate present queue
        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(g_GraphicsDevice->GetPhysicalDevice(), g_GraphicsDevice->GetGraphicsQueueFamilyIdx(), m_Surface, &present_support);
        RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, present_support == VK_TRUE, "The current VK graphics queue does not support presenting to the surface!");

        // Check surface capabilities
        VkSurfaceTransformFlagBitsKHR surface_transform;
        {
            VkSurfaceCapabilitiesKHR capabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_GraphicsDevice->GetPhysicalDevice(), m_Surface, &capabilities);

            if (capabilities.currentExtent.width != 0xFFFFFFFF)
            {
                m_Width  = capabilities.currentExtent.width;
                m_Height = capabilities.currentExtent.height;
            }
            else
            {
                m_Width  = Math::Clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                m_Height = Math::Clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            }

            if (capabilities.maxImageCount > 0)
                m_BackBufferCount = Math::Clamp(buffer_count, capabilities.minImageCount, capabilities.maxImageCount);
            else
                m_BackBufferCount = Math::Max(buffer_count, capabilities.minImageCount);

            bool alpha_support = (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) > 0;
            if (transparency_support && !alpha_support)
            {
                transparency_support = false;
                RB_LOG_WARN(LOGTAG_GRAPHICS, "Tried to enable transparency on the window while it doesn't support that, ignoring..");
            }

            surface_transform = capabilities.currentTransform;
        }

        // Check for supported formats
        VkSurfaceFormatKHR surface_format;
        {
            m_EngineFormat = format;

            std::vector<VkSurfaceFormatKHR> formats;

            uint32_t format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(g_GraphicsDevice->GetPhysicalDevice(), m_Surface, &format_count, nullptr);
            formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(g_GraphicsDevice->GetPhysicalDevice(), m_Surface, &format_count, formats.data());

            VkFormat target_format = ConvertToVKFormat(m_EngineFormat);

            bool format_found = false;
            for (VkSurfaceFormatKHR& compatible_format : formats)
            {
                if (compatible_format.format == target_format)
                {
                    surface_format = compatible_format;
                    format_found = true;
                    break;
                }
            }

            RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, format_found, "Specified swapchain format is not compatible with VK surface");
        }

        // Check for supported present modes
        VkPresentModeKHR present_mode;
        {
            uint32_t present_mode_count;
            std::vector<VkPresentModeKHR> present_modes;
            vkGetPhysicalDeviceSurfacePresentModesKHR(g_GraphicsDevice->GetPhysicalDevice(), m_Surface, &present_mode_count, nullptr);
            present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(g_GraphicsDevice->GetPhysicalDevice(), m_Surface, &present_mode_count, present_modes.data());

            if (!vsync && 
                std::find(present_modes.begin(), present_modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != present_modes.end())
            {
                present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
            else
            {
                // Mailbox is only really effective with 3 or more backbuffers
                if (m_BackBufferCount >= 3 &&
                    std::find(present_modes.begin(), present_modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != present_modes.end())
                {
                    present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                }
                else
                {
                    // This should always be available
                    present_mode = VK_PRESENT_MODE_FIFO_KHR;
                }
            }
        }

        // Create the actual swapchain
        VkSwapchainCreateInfoKHR info = {};
        info.sType               = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface             = m_Surface;
        info.minImageCount       = m_BackBufferCount;
        info.imageFormat         = surface_format.format;
        info.imageColorSpace     = surface_format.colorSpace;
        info.imageExtent         = { m_Width, m_Height };
        info.imageArrayLayers    = 1;
        info.imageUsage          = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.imageSharingMode    = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform        = surface_transform;
        info.compositeAlpha      = transparency_support ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode         = present_mode;
        info.clipped             = VK_TRUE;
        info.oldSwapchain        = VK_NULL_HANDLE;

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateSwapchainKHR(g_GraphicsDevice->Get(), &info, nullptr, &m_Swapchain), "Failed to create swapchain");

        m_CurrentBackBufferIndex = 0;
        m_UpdatedBackBufferIndex = true;

        m_WrappedBackBuffers = ALLOC_HEAPC(Texture2D*, m_BackBufferCount);
        for (int i = 0; i < m_BackBufferCount; ++i)
        {
            m_WrappedBackBuffers[i] = nullptr;
        }

        m_SwapChainImages = new VkImage[m_BackBufferCount];
        m_ImageViews = new VkImageView[m_BackBufferCount];

        // Get swap chain images
        uint32_t image_count;
        vkGetSwapchainImagesKHR(g_GraphicsDevice->Get(), m_Swapchain, &image_count, nullptr);
        RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, image_count == m_BackBufferCount, "There were not as many backbuffer images (%d) as expected (%d)", image_count, m_BackBufferCount);
        vkGetSwapchainImagesKHR(g_GraphicsDevice->Get(), m_Swapchain, &image_count, m_SwapChainImages);

        for (size_t i = 0; i < m_BackBufferCount; i++)
        {
            VkImageViewCreateInfo view_info = {};
            view_info.sType                             = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image                             = m_SwapChainImages[i];
            view_info.viewType                          = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format                            = surface_format.format;
            view_info.subresourceRange.aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.baseMipLevel     = 0;
            view_info.subresourceRange.levelCount       = 1;
            view_info.subresourceRange.baseArrayLayer   = 0;
            view_info.subresourceRange.layerCount       = 1;

            RB_ASSERT_FATAL_RELEASE_VK(
                vkCreateImageView(g_GraphicsDevice->Get(), &view_info, nullptr, &m_ImageViews[i]),
                "Failed to create backbuffer image views"
            );
        }
    }

    SwapChainVK::~SwapChainVK()
    {
        for (int i = 0; i < m_BackBufferCount; ++i)
        {
            SAFE_DELETE(m_WrappedBackBuffers[i]);
            
            vkDestroyImageView(g_GraphicsDevice->Get(), m_ImageViews[i], nullptr);
        }
        SAFE_FREE(m_WrappedBackBuffers);

        delete[] m_ImageViews;
        delete[] m_SwapChainImages;

        vkDestroySwapchainKHR(g_GraphicsDevice->Get(), m_Swapchain, nullptr);
        vkDestroySurfaceKHR(g_GraphicsDevice->GetInstance(), m_Surface, nullptr);
    }

    void SwapChainVK::Present()
    {
        UpdateBackBufferIndex();

        VkPresentInfoKHR present_info = {};
        present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 0; // Synchronization happens via API independent code
        present_info.pWaitSemaphores    = nullptr;
        present_info.swapchainCount     = 1;
        present_info.pSwapchains        = &m_Swapchain;
        present_info.pImageIndices      = &m_CurrentBackBufferIndex;
        present_info.pResults           = nullptr;

        VkResult result = vkQueuePresentKHR(g_GraphicsDevice->GetGraphicsQueue()->GetQueue(), &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            // Do we need this?
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "TODO: implement resize on out of date present");
            //Resize(m_Width, m_Height);
        }
        else
        {
            RB_ASSERT_FATAL_RELEASE_VK(result, "Failed to present swap chain image!");
        }

        m_UpdatedBackBufferIndex = false;
    }

    void SwapChainVK::Resize(const uint32_t width, const uint32_t height)
    {
        // Release wrapped backbuffer references
        for (int i = 0; i < m_BackBufferCount; ++i)
        {
            SAFE_DELETE(m_WrappedBackBuffers[i]);
        }

        //static_assert(false);
        RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "TODO: implement resize for VK swapchain");
    }

    Graphics::Texture2D* SwapChainVK::GetCurrentBackBuffer()
    {
        UpdateBackBufferIndex();

        if (m_WrappedBackBuffers[m_CurrentBackBufferIndex] == nullptr)
        {
            std::string name = "Backbuffer resource " + std::to_string(m_CurrentBackBufferIndex);

            m_WrappedBackBuffers[m_CurrentBackBufferIndex] = Texture2D::Create(
                name.c_str(),
                new GpuResource(m_SwapChainImages[m_CurrentBackBufferIndex], false),
                m_EngineFormat,
                m_Width,
                m_Height,
                true,
                false
            );

            ((Texture2DVK*)m_WrappedBackBuffers[m_CurrentBackBufferIndex])->SetView(m_ImageViews[m_CurrentBackBufferIndex]);
        }

        return m_WrappedBackBuffers[m_CurrentBackBufferIndex];
    }

    void SwapChainVK::UpdateBackBufferIndex()
    {
        if (m_UpdatedBackBufferIndex)
        {
            return;
        }

        RB_ASSERT_FATAL_RELEASE_VK(vkAcquireNextImageKHR(g_GraphicsDevice->Get(), m_Swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &m_CurrentBackBufferIndex),
                                    "Failed to acquire next backbuffer image");

        m_UpdatedBackBufferIndex = true;
    }
}
#endif