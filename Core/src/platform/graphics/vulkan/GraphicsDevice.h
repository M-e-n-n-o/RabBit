#if RB_GRAPHICS_API_VULKAN

#pragma once
#include "RabBitCommon.h"
#include "graphics/Renderer.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class GraphicsDevice
    {
    public:
        GraphicsDevice(bool debug_device);
        ~GraphicsDevice();

        static_assert(false);
        // TODO
        // - Create a wrapper around VkQueue (DeviceQueue) (check https://github.com/elecro/vkdemos/blob/master/vktriangle/vktriangle.cpp)

        VkQueue GetGraphicsQueue() const;
        VkQueue GetComputeQueue() const;
        VkQueue GetTransferQueue() const;

        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

    private:
        void CreateInstance(List<const char*> validation_layers);
        void CreateDevice(List<const char*> validation_layers);
        VkPhysicalDevice FindPhysicalDevice(UnorderedMap<VkQueueFlagBits, uint32_t>& queue_families);

        VkInstance          m_Instance;
        VkDevice            m_Device;
        VkPhysicalDevice    m_PhysicalDevice;

        VkQueue             m_GraphicsQueue;
        bool                m_HasComputeQueue;
        VkQueue             m_ComputeQueue;
        bool                m_HasTransferQueue;
        VkQueue             m_TransferQueue; // (copy queue)
    };

    extern GraphicsDevice* g_GraphicsDevice;
}
#endif