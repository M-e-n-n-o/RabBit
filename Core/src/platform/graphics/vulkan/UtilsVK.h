#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "GraphicsDevice.h"
#include "graphics/RenderResource.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    static inline VkFormat ConvertToVKFormat(const RenderResourceFormat& format)
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
        case (RenderResourceFormat::B8G8R8A8_UNORM):
            return VK_FORMAT_B8G8R8A8_UNORM;
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
        case(RenderResourceFormat::R32_TYPELESS):
            return VK_FORMAT_D32_SFLOAT; // D32_SFLOAT can be used for both depth & SRV
        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Format not yet supported");
            return VK_FORMAT_UNDEFINED;
        }
    }

    static inline VkImageSubresourceRange GetImageSubResourceRange(RenderResource* resource)
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

    static inline uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_props;
        vkGetPhysicalDeviceMemoryProperties(g_GraphicsDevice->GetPhysicalDevice(), &mem_props);

        for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
        {
            if ((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Failed to find suitable memory type");
        return 0;
    }

    static inline void SetObjectName(uint64_t handle, VkObjectType type, const char* name) 
    {
#ifdef RB_CONFIG_DEBUG
        VkDebugUtilsObjectNameInfoEXT name_info = {};
        name_info.sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectHandle   = handle;
        name_info.objectType     = type;
        name_info.pObjectName    = name;

        auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(g_GraphicsDevice->Get(), "vkSetDebugUtilsObjectNameEXT");
        if (func) 
        {
            func(g_GraphicsDevice->Get(), &name_info);
        }
#endif
    }

    static inline void GetAccessMasksForState(ResourceState state,
                                              VkAccessFlags& access_mask,
                                              VkPipelineStageFlags& stage_mask,
                                              VkImageLayout& layout)
    {
        switch (state)
        {
        case ResourceState::COMMON:
            access_mask = 0;
            stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            layout      = VK_IMAGE_LAYOUT_UNDEFINED;
            break;

        case ResourceState::RENDER_TARGET:
            access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;

        case ResourceState::DEPTH_WRITE:
            access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            break;

        case ResourceState::PIXEL_SHADER_RESOURCE:
            access_mask = VK_ACCESS_SHADER_READ_BIT;
            stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;

        case ResourceState::ALL_SHADER_RESOURCE:
            access_mask = VK_ACCESS_SHADER_READ_BIT;
            stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;

        case ResourceState::UNORDERED_ACCESS:
            access_mask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            layout      = VK_IMAGE_LAYOUT_GENERAL;
            break;

        case ResourceState::COPY_DEST:
            access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
            stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            break;

        case ResourceState::COPY_SOURCE:
            access_mask = VK_ACCESS_TRANSFER_READ_BIT;
            stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
            layout      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            break;

        case ResourceState::PRESENT:
            access_mask = 0;
            stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            layout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            break;

        default:
            RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Resource state not yet implemented");
        }
    }
}
#endif