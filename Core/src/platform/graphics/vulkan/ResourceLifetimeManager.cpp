#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "ResourceLifetimeManager.h"
#include "GpuResource.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::VK
{
    ResourceLifetimeManager* g_ResourceLifetimeManager = nullptr;
    
    ResourceLifetimeManager::ResourceLifetimeManager()
        : m_CurrentList(0)
    {
    }

    ResourceLifetimeManager::~ResourceLifetimeManager()
    {
        Flush();
    }

    void ResourceLifetimeManager::DeferReleaseBuffer(VkBuffer buffer, VkDeviceMemory memory)
    {
        Resource res = {};
        res.type    = GpuResourceType::Buffer;
        res.buffer  = buffer;
        res.memory  = memory;

        m_Resources[m_CurrentList].push_back(res);
    }

    void ResourceLifetimeManager::DeferReleaseImage(VkImage image, VkDeviceMemory memory)
    {
        Resource res = {};
        res.type    = GpuResourceType::Image;
        res.image   = image;
        res.memory  = memory;

        m_Resources[m_CurrentList].push_back(res);
    }

    void ResourceLifetimeManager::Update()
    {
        m_CurrentList = (m_CurrentList + 1) % TRANSIENT_CYCLES;

        auto& list = m_Resources[m_CurrentList];

        for (auto& res : list)
        {
            if (res.type == GpuResourceType::Image)
            {
                vkDestroyImage(g_GraphicsDevice->Get(), res.image, nullptr);
            }
            else
            {
                vkDestroyBuffer(g_GraphicsDevice->Get(), res.buffer, nullptr);
            }

            vkFreeMemory(g_GraphicsDevice->Get(), res.memory, nullptr);
        }

        list.clear();
    }

    void ResourceLifetimeManager::Flush()
    {
        for (int i = 0; i < TRANSIENT_CYCLES; ++i)
        {
            Update();
        }
    }
}
#endif