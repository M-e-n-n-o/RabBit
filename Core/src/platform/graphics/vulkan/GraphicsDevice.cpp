#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::VK
{
    GraphicsDevice* g_GraphicsDevice = nullptr;

    GraphicsDevice::GraphicsDevice(bool debug_device)
    {
        List<const char*> validation_layers;

        if (debug_device)
        {
            validation_layers.emplace_back("VK_LAYER_KHRONOS_validation");
        }

        CreateInstance(validation_layers);
        CreateDevice(validation_layers);
    }

    GraphicsDevice::~GraphicsDevice()
    {

    }

    void GraphicsDevice::CreateInstance(List<const char*> validation_layers)
    {
        std::vector<const char*> extensions{};

        // If a debug callbacks should be enabled:
        //  * The extension must be specified and
        //  * The "pNext" should point to a valid "VkDebugUtilsMessengercreate_infoEXT" struct.
        // extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        //create_info.pNext = (VkDebugUtilsMessengercreate_infoEXT*) &debugInfo;

        VkApplicationInfo app_info;
        app_info.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext               = NULL;
        app_info.pApplicationName    = "RabBit App";
        app_info.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName         = "RabBit";
        app_info.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion          = VK_API_VERSION_1_2;

        VkInstanceCreateInfo create_info;
        create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pNext                   = NULL;
        create_info.flags                   = 0;
        create_info.pApplicationInfo        = &app_info;
        create_info.enabledLayerCount       = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames     = validation_layers.data();
        create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateInstance(&create_info, NULL, &m_Instance), "Failed to create VK instance");
    }

    void GraphicsDevice::CreateDevice(List<const char*> validation_layers)
    {

    }
    
    VkPhysicalDevice GraphicsDevice::FindPhysicalDevice()
    {
        // TODO:
        // - Enumerate over all the physical devices and check for:
        //      - VK 1.2 support
        //      - Support for 1 graphics queue, 1 compute queue & 1 copy queue (I think?)
        //      - Out of these, choose the device with the most VRAM

        return VkPhysicalDevice();
    }
}
#endif