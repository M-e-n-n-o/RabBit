#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "RabBitCommon.h"
#include "RendererVK.h"
#include "GpuResource.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class ResourceLifetimeManager
    {
    public:
        ResourceLifetimeManager();
        ~ResourceLifetimeManager();

        void DeferReleaseBuffer(VkBuffer buffer, VkDeviceMemory memory);
        void DeferReleaseImage(VkImage image, VkDeviceMemory memory);

        void Update();
        void Flush();

    private:
        struct Resource
        {
            union
            {
                VkBuffer buffer;
                VkImage  image;
            };
            GpuResourceType type;
            VkDeviceMemory  memory;
        };

        List<Resource> m_Resources[TRANSIENT_CYCLES];
        uint32_t       m_CurrentList;
    };

    extern ResourceLifetimeManager* g_ResourceLifetimeManager;
}
#endif