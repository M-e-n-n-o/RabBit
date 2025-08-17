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

    void GpuResource::UpdateState(ResourceState new_state)
    {
        RB_ASSERT(LOGTAG_GRAPHICS, (uint32_t)new_state <= 15, "The m_CurrentState variable should get more bits");
        m_CurrentState = (uint8_t)new_state;
    }

    ResourceState GpuResource::GetState() const
    {
        return (ResourceState)m_CurrentState;
    }
    
    VkBuffer GpuResource::GetNativeBuffer() const
    {
        if (m_ResourceType != (uint8_t)GpuResourceType::Buffer)
        {
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Cannot retrieve native buffer as this resource is not of this type");
            return nullptr;
        }

        return m_Buffer;
    }
    
    VkImage GpuResource::GetNativeImage() const
    {
        if (m_ResourceType != (uint8_t)GpuResourceType::Image)
        {
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Cannot retrieve native image as this resource is not of this type");
            return nullptr;
        }

        return m_Image;
    }
}
#endif