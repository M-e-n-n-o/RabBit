#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "GpuResource.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::VK
{
    GpuResource::GpuResource(const VkImage& image, bool transfer_ownership)
        : m_Image(image)
        , m_ResourceType((uint8_t)GpuResourceType::Image)
        , m_OwnsResource(transfer_ownership)
        , m_IsValid(true)
    {
    }

    GpuResource::~GpuResource()
    {
        if (!IsValid() || !m_OwnsResource)
        {
            return;
        }

        m_IsValid = false;

        if (m_ResourceType == (uint8_t)GpuResourceType::Image)
        {
            vkDestroyImage(g_GraphicsDevice->Get(), m_Image, nullptr);
        }
        else
        {
            vkDestroyBuffer(g_GraphicsDevice->Get(), m_Buffer, nullptr);
        }
    }
}
#endif