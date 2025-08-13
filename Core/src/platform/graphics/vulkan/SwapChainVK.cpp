#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "SwapChainVK.h"
#include "GraphicsDevice.h"
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
#endif
    
    Graphics::Texture2D* SwapChainVK::GetCurrentBackBuffer()
    {
        return nullptr;
    }

    void SwapChainVK::Init(uint32_t width, uint32_t height, bool vsync, uint32_t buffer_count, RenderResourceFormat format, bool transparency_support)
    {
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

            static_assert(false);
            // TODO:
            // - Some of this stuff, especially VkSurfaceCapabilitiesKHR should probably be put into DisplayVK

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
        info.imageUsage          = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.imageSharingMode    = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform        = surface_transform;
        info.compositeAlpha      = transparency_support ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode         = present_mode;
        info.clipped             = VK_TRUE;
        info.oldSwapchain        = VK_NULL_HANDLE;

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateSwapchainKHR(g_GraphicsDevice->Get(), &info, nullptr, &m_Swapchain), "Failed to create swapchain");
    }

    SwapChainVK::~SwapChainVK()
    {
        vkDestroySwapchainKHR(g_GraphicsDevice->Get(), m_Swapchain, nullptr);
        vkDestroySurfaceKHR(g_GraphicsDevice->GetInstance(), m_Surface, nullptr);
    }

    void SwapChainVK::Present()
    {
        static_assert(false);
    }

    void SwapChainVK::Resize(const uint32_t width, const uint32_t height)
    {
        static_assert(false);
    }
}
#endif