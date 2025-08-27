#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "platform/windowing/SwapChain.h"
#include "graphics/RenderResource.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class SwapChainVK : public SwapChain
    {
    public:
#if RB_PLATFORM_WINDOWS
        SwapChainVK(HWND window_handle, HINSTANCE h_instance, uint32_t width, uint32_t height, bool vsync, uint32_t buffer_count, RenderResourceFormat format, bool transparency_support);
#elif RB_PLATFORM_LINUX_ES
        SwapChainVK(uint32_t width, uint32_t height, uint32_t buffer_count, RenderResourceFormat format, bool transparency_support);
#endif
        ~SwapChainVK();

        void Present() override;
        void Resize(const uint32_t width, const uint32_t height) override;

        void* GetNativeSwapChain() const override { return (void*)&m_Swapchain; }
        uint32_t GetWidth() override { return m_Width; }
        uint32_t GetHeight() override { return m_Height; }
        uint32_t GetBackBufferCount() override { return m_BackBufferCount; }
        uint32_t GetCurrentBackBufferIndex() override { return m_CurrentBackBufferIndex; }
        Graphics::Texture2D* GetCurrentBackBuffer() override;

    private:
        void Init(uint32_t width, uint32_t height, bool vsync, uint32_t buffer_count, RenderResourceFormat format, bool transparency_support);
        void UpdateBackBufferIndex();

        VkSurfaceKHR            m_Surface;
        VkSwapchainKHR          m_Swapchain;

        uint32_t                m_Width;
        uint32_t                m_Height;
        RenderResourceFormat    m_EngineFormat;

        uint32_t                m_CurrentBackBufferIndex;
        uint32_t                m_BackBufferCount;
        bool                    m_UpdatedBackBufferIndex;

        VkImage*                m_SwapChainImages;
        VkImageView*            m_ImageViews;
        Graphics::Texture2D**   m_WrappedBackBuffers;
    };
}
#endif