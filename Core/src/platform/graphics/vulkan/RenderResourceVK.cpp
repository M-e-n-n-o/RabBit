#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "RenderResourceVK.h"
#include "GpuResource.h"
#include "UtilsVK.h"

namespace RB::Graphics::VK
{
    Texture2DVK::Texture2DVK(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access)
        : m_Name(name)
        , m_Format(format)
        , m_Width(width)
        , m_Height(height)
        , m_IsRenderTarget(is_render_target)
        , m_AllowReadWrite(random_read_write_access)
    {
        m_IsDepthStencil = IsDepthFormat(format);

        VkImageCreateInfo info = {};
        info.sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.flags          = 0;
        info.imageType      = VK_IMAGE_TYPE_2D;
        info.format         = ConvertToVKFormat(format);
        info.extent         = { width, height, 1 };
        info.mipLevels      = 1;
        info.arrayLayers    = 1;
        info.samples        = VK_SAMPLE_COUNT_1_BIT;
        info.tiling         = VK_IMAGE_TILING_OPTIMAL;
        info.usage          = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_SAMPLED_BIT;
        info.sharingMode    = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;

        if (m_IsRenderTarget)
            info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (m_AllowReadWrite)
            info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (m_IsDepthStencil)
            info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        m_Resource = new GpuResource(name, info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ResourceState::COMMON);
    }

    Texture2DVK::Texture2DVK(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access)
        : m_Name(name)
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
    
    uint32_t Texture2DVK::GetMipCount() const
    {
        // TODO Add mip support
        return 1;
    }

    uint32_t Texture2DVK::GetBaseMip() const
    {
        // TODO Add mip support
        return 0;
    }

    void Texture2DVK::SetBaseMip(uint32_t mip)
    {
        // TODO Add mip support
    }

    void Texture2DVK::SetMipCount(uint32_t mips)
    {
        // TODO Add mip support
    }

    void Texture2DVK::SetView(VkImageView image_view)
    {
        m_ImageView = image_view;
    }
}
#endif