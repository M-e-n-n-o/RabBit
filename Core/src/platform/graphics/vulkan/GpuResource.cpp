#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "GpuResource.h"
#include "GraphicsDevice.h"
#include "UtilsVK.h"

namespace RB::Graphics::VK
{
    GpuResource(const char* name, const VkBufferCreateInfo& buffer_create_info, VkMemoryPropertyFlagBits memory_type, ResourceState state)
        : m_ResourceType((uint8_t)GpuResourceType::Buffer)
        , m_OwnsResource(true)
        , m_IsValid(true)
        , m_CurrentState((uint8_t)state)
    {
        const VkDevice& device = g_GraphicsDevice->Get();

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateBuffer(device, &buffer_create_info, nullptr, &m_Buffer), "Failed to create buffer");

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(device, m_Buffer, &mem_reqs);

        uint32_t memoryTypeIndex = FindMemoryType(mem_reqs.memoryTypeBits, memory_type);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType            = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext            = NULL;
        alloc_info.allocationSize   = mem_reqs.size;
        alloc_info.memoryTypeIndex  = memoryTypeIndex;

        RB_ASSERT_FATAL_RELEASE_VK(vkAllocateMemory(device, &alloc_info, NULL, &m_Memory), "Failed to allocate buffer memory");

        vkBindImageMemory(device, m_Buffer, m_Memory, 0);

        SetObjectName((uint64_t)m_Buffer, VK_OBJECT_TYPE_BUFFER, name);
    }

    GpuResource::GpuResource(const char* name, const VkImageCreateInfo& image_create_info, VkMemoryPropertyFlagBits memory_type, ResourceState state)
        : m_ResourceType((uint8_t)GpuResourceType::Image)
        , m_OwnsResource(true)
        , m_IsValid(true)
        , m_CurrentState((uint8_t)state)
    {
        const VkDevice& device = g_GraphicsDevice->Get();

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateImage(device, &image_create_info, nullptr, &m_Image), "Failed to create image");

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(device, m_Image, &mem_reqs);

        uint32_t memoryTypeIndex = FindMemoryType(mem_reqs.memoryTypeBits, memory_type);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType            = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext            = NULL;
        alloc_info.allocationSize   = mem_reqs.size;
        alloc_info.memoryTypeIndex  = memoryTypeIndex;

        RB_ASSERT_FATAL_RELEASE_VK(vkAllocateMemory(device, &alloc_info, NULL, &m_Memory), "Failed to allocate image memory");

        vkBindImageMemory(device, m_Image, m_Memory, 0);

        SetObjectName((uint64_t)m_Image, VK_OBJECT_TYPE_IMAGE, name);
    }

    GpuResource::GpuResource(const char* name, const VkImage& image, ResourceState state, bool transfer_ownership)
        : m_Image(image)
        , m_ResourceType((uint8_t)GpuResourceType::Image)
        , m_OwnsResource(transfer_ownership)
        , m_IsValid(true)
        , m_CurrentState((uint8_t)state)
    {
        SetObjectName((uint64_t)m_Image, VK_OBJECT_TYPE_IMAGE, name);
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

        vkFreeMemory(g_GraphicsDevice->Get(), m_Memory, nullptr);
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
            return VK_NULL_HANDLE;
        }

        return m_Buffer;
    }
    
    VkImage GpuResource::GetNativeImage() const
    {
        if (m_ResourceType != (uint8_t)GpuResourceType::Image)
        {
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Cannot retrieve native image as this resource is not of this type");
            return VK_NULL_HANDLE;
        }

        return m_Image;
    }
}
#endif