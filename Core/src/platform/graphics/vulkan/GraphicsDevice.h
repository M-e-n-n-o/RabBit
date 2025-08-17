#if RB_GRAPHICS_API_VULKAN

#pragma once
#include "RabBitCommon.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class DeviceQueue;

    class GraphicsDevice
    {
    public:
        GraphicsDevice(bool debug_device);
        ~GraphicsDevice();

        DeviceQueue* GetGraphicsQueue() const;
        DeviceQueue* GetComputeQueue() const;
        DeviceQueue* GetTransferQueue() const;

        VkDevice Get() const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkInstance GetInstance() const { return m_Instance; }

    private:
        void CreateInstance(bool debug_device, List<const char*> validation_layers);
        void CreateDevice(List<const char*> validation_layers);

        VkPhysicalDevice FindPhysicalDevice(UnorderedMap<VkQueueFlagBits, uint32_t>& queue_families);

        void ValidateLayers(List<const char*>& layers);
        void ValidateInstanceExtensions(List<const char*>& extensions);
        void ValidateDeviceExtensions(List<const char*>& extensions);

        VkInstance                  m_Instance;
        VkDevice                    m_Device;
        VkPhysicalDevice            m_PhysicalDevice;
#ifdef RB_CONFIG_DEBUG
        VkDebugUtilsMessengerEXT    m_DebugMessenger;
#endif

        DeviceQueue*                m_GraphicsQueue;
        DeviceQueue*                m_ComputeQueue;
        DeviceQueue*                m_TransferQueue; // (copy queue)
    };

    extern GraphicsDevice* g_GraphicsDevice;
}
#endif