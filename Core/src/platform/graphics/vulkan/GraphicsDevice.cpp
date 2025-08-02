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

    VkQueue GraphicsDevice::GetGraphicsQueue() const
    {
        return m_GraphicsQueue;
    }

    VkQueue GraphicsDevice::GetComputeQueue() const
    {
        if (m_HasComputeQueue)
            return m_ComputeQueue;

        return m_GraphicsQueue;
    }

    VkQueue GraphicsDevice::GetTransferQueue() const
    {
        if (m_HasTransferQueue)
            return m_TransferQueue;

        return m_GraphicsQueue;
    }

    void GraphicsDevice::CreateInstance(List<const char*> validation_layers)
    {
        std::vector<const char*> extensions = {

#if !RB_PLATFORM_WINDOWS
            // For enumerating connected display's using the Vulkan API (not supported on Windows)
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_DISPLAY_EXTENSION_NAME
#endif
        };

        // If a debug callbacks should be enabled:
        //  * The extension must be specified and
        //  * The "pNext" should point to a valid "VkDebugUtilsMessengercreate_infoEXT" struct.
        // extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        //create_info.pNext = (VkDebugUtilsMessengercreate_infoEXT*) &debugInfo;

        VkApplicationInfo app_info = {};
        app_info.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext               = NULL;
        app_info.pApplicationName    = "RabBit App";
        app_info.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName         = "RabBit";
        app_info.engineVersion       = VK_MAKE_VERSION(0, 1, 0);
        app_info.apiVersion          = VK_API_VERSION_1_2;

        VkInstanceCreateInfo create_info = {};
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
        UnorderedMap<VkQueueFlagBits, uint32_t> queue_families;
        m_PhysicalDevice = FindPhysicalDevice(queue_families);

        float queue_priority = 1.0f;
        List<VkDeviceQueueCreateInfo> queue_create_infos;

        for (const auto& family : queue_families) 
        {
            VkDeviceQueueCreateInfo queue_info = {};
            queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info.flags            = 0;
            queue_info.queueFamilyIndex = family.second;
            queue_info.queueCount       = 1;
            queue_info.pQueuePriorities = &queue_priority;

            queue_create_infos.push_back(queue_info);
        }

        VkDeviceCreateInfo info;
        info.sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext                      = NULL;
        info.flags                      = 0;
        info.queueCreateInfoCount       = static_cast<uint32_t>(queue_create_infos.size());
        info.pQueueCreateInfos          = queue_create_infos.data();
        info.pEnabledFeatures           = NULL;
        info.enabledExtensionCount      = 0;
        info.ppEnabledExtensionNames    = NULL;
        info.enabledLayerCount          = static_cast<uint32_t>(validation_layers.size());
        info.ppEnabledLayerNames        = validation_layers.data();

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateDevice(m_PhysicalDevice, &info, NULL, &m_Device), "Failed to create VK device");

        // Get the several dedicated queue's
        vkGetDeviceQueue(m_Device, queue_families[VK_QUEUE_GRAPHICS_BIT], 0, &m_GraphicsQueue);
        
        if (auto compute_itr = queue_families.find(VK_QUEUE_COMPUTE_BIT); compute_itr != queue_families.end())
        {
            m_HasComputeQueue = true;
            vkGetDeviceQueue(m_Device, compute_itr->second, 0, &m_ComputeQueue);
        }
        else
        {
            m_HasComputeQueue = false;
        }

        if (auto transfer_itr = queue_families.find(VK_QUEUE_TRANSFER_BIT); transfer_itr != queue_families.end())
        {
            m_HasTransferQueue = true;
            vkGetDeviceQueue(m_Device, transfer_itr->second, 0, &m_TransferQueue);
        }
        else
        {
            m_HasTransferQueue = false;
        }
    }
    
    VkPhysicalDevice GraphicsDevice::FindPhysicalDevice(UnorderedMap<VkQueueFlagBits, uint32_t>& queue_families)
    {
        RB_LOG(LOGTAG_GRAPHICS, "-------- ADAPTER INFORMATION --------");

        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_Instance, &device_count, nullptr);

        if (device_count == 0) 
        {
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Failed to find a GPU with Vulkan 1.2 support!");
        }

        List<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(m_Instance, &device_count, devices.data());

        RB_LOG(LOGTAG_GRAPHICS, "Found the following Vulkan compatible GPU's:");

        int preferred_device_idx = -1;
        VkDeviceSize preferred_vram_size = 0;
        std::string preferred_name;
        int preferred_graphics_family = -1;
        int preferred_compute_family = -1;
        int preferred_transfer_family = -1;
        for (int device_idx = 0; device_idx < devices.size(); ++device_idx)
        {
            const VkPhysicalDevice& device = devices[device_idx];

            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

            List<VkQueueFamilyProperties> queue_families(queue_family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

            int graphics_family = -1;
            int compute_family  = -1;
            int transfer_family = -1;
            for (int i = 0; i < queue_families.size(); ++i)
            {
                VkQueueFlags flags = queue_families[i].queueFlags;

                // Check for dedicated graphics, compute & transfer (copy) queues

                if ((flags & VK_QUEUE_GRAPHICS_BIT) && graphics_family == -1)
                    graphics_family = i;

                if ((flags & VK_QUEUE_COMPUTE_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT) && compute_family == -1)
                    compute_family = i;

                if ((flags & VK_QUEUE_TRANSFER_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT) && !(flags & VK_QUEUE_COMPUTE_BIT) && transfer_family == -1)
                    transfer_family = i;
            }

            if (graphics_family == -1)
            {
                // We need at least a dedicated graphics queue
                continue;
            }

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            // Print the device name
            RB_LOG(LOGTAG_GRAPHICS, "\t%d. %s", device_idx + 1, properties.deviceName);

            // Choose the device with the most VRAM
            VkPhysicalDeviceMemoryProperties mem_props;
            vkGetPhysicalDeviceMemoryProperties(device, &mem_props);

            VkDeviceSize dedicated_vram_size = 0;
            for (uint32_t i = 0; i < mem_props.memoryHeapCount; i++)
            {
                const VkMemoryHeap& heap = mem_props.memoryHeaps[i];

                bool host_visible = false;

                // Check if any memory type for this heap is host-visible
                for (uint32_t j = 0; j < mem_props.memoryTypeCount; j++) 
                {
                    if (mem_props.memoryTypes[j].heapIndex == i && (mem_props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) 
                    {
                        host_visible = true;
                        break;
                    }
                }

                if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) && !host_visible)
                {
                    dedicated_vram_size += heap.size;
                }
            }

            if (dedicated_vram_size >= preferred_vram_size)
            {
                preferred_device_idx        = device_idx;
                preferred_vram_size         = dedicated_vram_size;
                preferred_name              = properties.deviceName;
                preferred_graphics_family   = graphics_family;
                preferred_compute_family    = compute_family;
                preferred_transfer_family   = transfer_family;
            }
        }

        RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, preferred_device_idx >= 0, "Could not find any Vulkan compatible GPU");

        RB_LOG(LOGTAG_GRAPHICS, "Selected graphics device:");
        RB_LOG(LOGTAG_GRAPHICS, "\tName: %s", preferred_name.c_str());
        RB_LOG(LOGTAG_GRAPHICS, "\tVRAM: %d MB", (preferred_vram_size / (1024 * 1024)));

        queue_families.emplace(VK_QUEUE_GRAPHICS_BIT, preferred_graphics_family);
        if (preferred_compute_family >= 0)
            queue_families.emplace(VK_QUEUE_COMPUTE_BIT, preferred_compute_family);
        if (preferred_transfer_family >= 0)
            queue_families.emplace(VK_QUEUE_TRANSFER_BIT, preferred_transfer_family);

        return devices[preferred_device_idx];
    }
}
#endif