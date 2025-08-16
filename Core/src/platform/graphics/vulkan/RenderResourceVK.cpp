#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "RenderResourceVK.h"
#include "GpuResource.h"

namespace RB::Graphics::VK
{
    Texture2DVK::Texture2DVK(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space)
        : Texture2D(color_space)
        , m_Name(name)
        , m_Resource((GpuResource*)internal_resource)
        , m_Format(format)
        , m_Width(width)
        , m_Height(height)
        , m_IsRenderTarget(is_render_target)
        , m_AllowReadWrite(random_read_write_access)
    {
        m_IsDepthStencil = IsDepthFormat(format);
    }

    Texture2DVK::~Texture2DVK()
    {
        SAFE_DELETE(m_Resource);
    }
    
    void Texture2DVK::SetView(VkImageView image_view)
    {
        m_ImageView = image_view;
    }
}
#endif