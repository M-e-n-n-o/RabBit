#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "graphics/RenderResource.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    static VkFormat ConvertToVKFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case(RenderResourceFormat::R32G32B32A32_FLOAT):
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case(RenderResourceFormat::R16G16B16A16_FLOAT):
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case(RenderResourceFormat::R32G32_FLOAT):
            return VK_FORMAT_R32G32_SFLOAT;
        case(RenderResourceFormat::R32_UINT):
            return VK_FORMAT_R32_UINT;
        case(RenderResourceFormat::R8G8B8A8_SRGB):
            return VK_FORMAT_R8G8B8A8_SRGB;
        case(RenderResourceFormat::R8G8B8A8_UNORM):
            return VK_FORMAT_R8G8B8A8_UNORM;
        case(RenderResourceFormat::R16G16_FLOAT):
            return VK_FORMAT_R16G16_SFLOAT;
        case(RenderResourceFormat::R16G16_UINT):
            return VK_FORMAT_R16G16_UINT;
        case(RenderResourceFormat::R16_FLOAT):
            return VK_FORMAT_R16_SFLOAT;
        case(RenderResourceFormat::R16_UINT):
            return VK_FORMAT_R16_UINT;
        case(RenderResourceFormat::R16_UNORM):
            return VK_FORMAT_R16_UNORM;
        case(RenderResourceFormat::R16_SNORM):
            return VK_FORMAT_R16_SNORM;
        case(RenderResourceFormat::R8_UNORM):
            return VK_FORMAT_R8_UNORM;
        case(RenderResourceFormat::R32_FLOAT):
            return VK_FORMAT_R32_SFLOAT;
        case(RenderResourceFormat::R8_UINT):
            return VK_FORMAT_R8_UINT;
        case(RenderResourceFormat::D32_FLOAT):
            return VK_FORMAT_D32_SFLOAT;
        case(RenderResourceFormat::D16_UNORM):
            return VK_FORMAT_D16_UNORM;
        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Format not yet supported");
            return VK_FORMAT_UNDEFINED;
        }
    }

    static VkImageSubresourceRange GetImageSubResourceRange(RenderResource* resource)
    {
        if (resource->GetPrimitiveType() != RenderResourceType::Texture)
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Cannot get the subresource range of an image that is not of the texture type");
            return {};
        }

        Texture* tex = ((Texture*)resource);

        VkImageSubresourceRange range = {};
        range.aspectMask     = tex->AllowedDepthStencil() ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_COLOR_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = tex->GetBaseMip();
        range.levelCount     = tex->GetMipCount();
        range.baseArrayLayer = tex->GetFirstArraySlice();
        range.layerCount     = tex->GetArraySize();

        return range;
    }
}
#endif