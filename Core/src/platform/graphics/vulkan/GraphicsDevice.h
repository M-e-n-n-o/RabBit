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

    private:
        void CreateInstance(List<const char*> validation_layers);
        void CreateDevice(List<const char*> validation_layers);
        VkPhysicalDevice FindPhysicalDevice();

        VkInstance  m_Instance;
    };

    extern GraphicsDevice* g_GraphicsDevice;
}
#endif