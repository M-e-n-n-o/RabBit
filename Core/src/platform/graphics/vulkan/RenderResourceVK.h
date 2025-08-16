#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class GpuResource;

    class Texture2DVK : public Texture2D
    {
    public:
        Texture2DVK(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space);
        ~Texture2DVK();

        const char* GetName() const override { return m_Name; }

        void* GetNativeResource() const override { return m_Resource; }

        RenderResourceFormat GetFormat() const override { return m_Format; }

        bool AllowedRenderTarget() const override { return m_IsRenderTarget; }
        bool AllowedDepthStencil() const override { return m_IsDepthStencil; }
        bool AllowedRandomReadWrites() const override { return m_AllowReadWrite; }

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }

        void SetView(VkImageView image_view);

    private:
        const char*                     m_Name;
        GpuResource*                    m_Resource;
        VkImageView                     m_ImageView;
        uint32_t                        m_Width;
        uint32_t                        m_Height;
        RenderResourceFormat            m_Format;

        bool                            m_IsRenderTarget;
        bool                            m_IsDepthStencil;
        bool                            m_AllowReadWrite;
    };
}
#endif