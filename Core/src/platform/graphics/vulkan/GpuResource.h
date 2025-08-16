#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "RabBitCommon.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    enum class GpuResourceType
    {
        Image,
        Buffer
    };

    class GpuResource
    {
    public:
        GpuResource(const VkImage& image, bool transfer_ownership);
        ~GpuResource();

        bool IsValid() const { return m_IsValid; }

    private:
        union
        {
            VkImage     m_Image;
            VkBuffer    m_Buffer;
        };

        VkDeviceMemory  m_Memory;

        uint8_t         m_ResourceType  : 1;
        uint8_t         m_OwnsResource  : 1;
        uint8_t         m_IsValid       : 1;
        uint8_t         m_Unused        : 5;
    };
}
#endif