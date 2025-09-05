#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "GraphicsDevice.h"
#include "DeviceQueue.h"

namespace RB::Graphics::VK
{
    GraphicsDevice* g_GraphicsDevice = nullptr;


    List<const char*> g_InstanceExtensions =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,

#if RB_PLATFORM_WINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif RB_PLATFORM_LINUX_ES
        // Used for both window creation on Linux, but also for enumerating
        // connected display's using the Vulkan API (not supported on Windows)
        VK_KHR_DISPLAY_EXTENSION_NAME
#endif
    };

    List<const char*> g_DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

#ifdef RB_CONFIG_DEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData) {

        switch (severity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            //RB_LOG(LOGTAG_GRAPHICS, "VK validation verbose: %s", callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            //RB_LOG(LOGTAG_GRAPHICS, "VK validation info: %s", callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "VK validation warning: %s", callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "VK validation error: %s", callbackData->pMessage);
            break;
        default:
            break;
        }

        return VK_FALSE;
    }
#endif

    GraphicsDevice::GraphicsDevice(bool debug_device)
    {
        List<const char*> validation_layers;

#ifdef RB_CONFIG_DEBUG
        if (debug_device)
        {
            validation_layers.emplace_back("VK_LAYER_KHRONOS_validation");
        }
#endif

        ValidateLayers(validation_layers);

        CreateInstance(debug_device, validation_layers);
        CreateDevice(validation_layers);
    }

    GraphicsDevice::~GraphicsDevice()
    {
        delete m_GraphicsQueue;
        if (m_ComputeQueue)
            delete m_ComputeQueue;
        if (m_TransferQueue)
            delete m_TransferQueue;

        vkDestroyDevice(m_Device, nullptr);

#ifdef RB_CONFIG_DEBUG
        if (m_DebugMessenger != VK_NULL_HANDLE)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
            vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, NULL);
        }
#endif

        vkDestroyInstance(m_Instance, nullptr);
    }

    DeviceQueue* GraphicsDevice::GetGraphicsQueue() const
    {
        return m_GraphicsQueue;
    }

    DeviceQueue* GraphicsDevice::GetComputeQueue() const
    {
        if (m_ComputeQueue)
            return m_ComputeQueue;

        return m_GraphicsQueue;
    }

    DeviceQueue* GraphicsDevice::GetTransferQueue() const
    {
        if (m_TransferQueue)
            return m_TransferQueue;

        return m_GraphicsQueue;
    }

    void GraphicsDevice::CreateInstance(bool debug_device, List<const char*> validation_layers)
    {
#ifdef RB_CONFIG_DEBUG
        VkDebugUtilsMessengerCreateInfoEXT msg_info = {};

        if (debug_device)
        {
            g_InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            msg_info.sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            msg_info.pNext              = nullptr;
            msg_info.messageSeverity    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            msg_info.messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            msg_info.pfnUserCallback    = DebugCallback;
        }
#endif

        ValidateInstanceExtensions(g_InstanceExtensions);

        VkApplicationInfo app_info = {};
        app_info.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext               = nullptr;
        app_info.pApplicationName    = "RabBit App";
        app_info.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName         = "RabBit";
        app_info.engineVersion       = VK_MAKE_VERSION(0, 1, 0);
        app_info.apiVersion          = VK_API_VERSION_1_2;

        VkInstanceCreateInfo create_info = {};
        create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef RB_CONFIG_DEBUG
        create_info.pNext                   = debug_device ? & msg_info : nullptr;
#else
        create_info.pNext                   = nullptr;
#endif
        create_info.flags                   = 0;
        create_info.pApplicationInfo        = &app_info;
        create_info.enabledLayerCount       = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames     = validation_layers.data();
        create_info.enabledExtensionCount   = static_cast<uint32_t>(g_InstanceExtensions.size());
        create_info.ppEnabledExtensionNames = g_InstanceExtensions.data();

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateInstance(&create_info, NULL, &m_Instance), "Failed to create VK instance");

#ifdef RB_CONFIG_DEBUG
        if (debug_device)
        {
            PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");

            if (vkCreateDebugUtilsMessengerEXT)
            {
                RB_ASSERT_FATAL_RELEASE_VK(vkCreateDebugUtilsMessengerEXT(m_Instance, &msg_info, nullptr, &m_DebugMessenger), "Failed to create VK debug messenger");
            }
            else
            {
                RB_LOG_ERROR(LOGTAG_GRAPHICS, "Failed to load vkCreateDebugUtilsMessengerEXT, there will be no validation messages");
            }
        }
        else
        {
            m_DebugMessenger = VK_NULL_HANDLE;
        }

#endif
    }

    void GraphicsDevice::CreateDevice(List<const char*> validation_layers)
    {
        UnorderedMap<VkQueueFlagBits, uint32_t> queue_families;
        m_PhysicalDevice = FindPhysicalDevice(queue_families);

        m_GraphicsQueueFamilyIdx = queue_families[VK_QUEUE_GRAPHICS_BIT];

        ValidateDeviceExtensions(g_DeviceExtensions);

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

        VkPhysicalDeviceTimelineSemaphoreFeatures timeline_features = {};
        {
            timeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;

            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &timeline_features;

            vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &features);

            RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, timeline_features.timelineSemaphore, "Device doesnt support timeline semaphore, which is required");
        }

        VkDeviceCreateInfo info = {};
        info.sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext                      = &timeline_features;
        info.flags                      = 0;
        info.queueCreateInfoCount       = static_cast<uint32_t>(queue_create_infos.size());
        info.pQueueCreateInfos          = queue_create_infos.data();
        info.pEnabledFeatures           = nullptr;
        info.enabledExtensionCount      = static_cast<uint32_t>(g_DeviceExtensions.size());
        info.ppEnabledExtensionNames    = g_DeviceExtensions.data();
        info.enabledLayerCount          = static_cast<uint32_t>(validation_layers.size());
        info.ppEnabledLayerNames        = validation_layers.data();

        RB_ASSERT_FATAL_RELEASE_VK(vkCreateDevice(m_PhysicalDevice, &info, nullptr, &m_Device), "Failed to create VK device");

        // Get the several dedicated queue's
        m_GraphicsQueue = new DeviceQueue(m_Device, VK_QUEUE_GRAPHICS_BIT, queue_families[VK_QUEUE_GRAPHICS_BIT], 0);
        
        if (auto compute_itr = queue_families.find(VK_QUEUE_COMPUTE_BIT); compute_itr != queue_families.end())
            m_ComputeQueue = new DeviceQueue(m_Device, VK_QUEUE_COMPUTE_BIT, compute_itr->second, 0);
        else
            m_ComputeQueue = nullptr;

        if (auto transfer_itr = queue_families.find(VK_QUEUE_TRANSFER_BIT); transfer_itr != queue_families.end())
            m_TransferQueue = new DeviceQueue(m_Device, VK_QUEUE_TRANSFER_BIT, transfer_itr->second, 0);
        else
            m_TransferQueue = nullptr;
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

            if (VK_VERSION_MAJOR(properties.apiVersion) < 1 ||
                (VK_VERSION_MAJOR(properties.apiVersion) == 1 && VK_VERSION_MINOR(properties.apiVersion) < 2))
            {
                // Need at least Vulkan 1.2 support
                continue;
            }

            if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
                properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                // Device type not valid
                continue;
            }

            std::string device_name = properties.deviceName;
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                device_name += " (integrated)";

            // Print the device name
            RB_LOG(LOGTAG_GRAPHICS, "\t%d. %s", device_idx + 1, device_name.c_str());

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
                preferred_name              = device_name;
                preferred_graphics_family   = graphics_family;
                preferred_compute_family    = compute_family;
                preferred_transfer_family   = transfer_family;
            }
        }

        RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, preferred_device_idx >= 0, "Could not find any Vulkan 1.2 compatible GPU");

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

    void GraphicsDevice::ValidateLayers(List<const char*>& layers)
    {
        if (layers.empty())
        {
            return;
        }

        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        List<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        // Remove unsupported layers
        auto it = layers.begin();
        while (it != layers.end())
        {
            bool layer_found = false;
            for (const auto& layer_properties : available_layers)
            {
                if (strcmp(*it, layer_properties.layerName) == 0)
                {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found)
            {
                RB_LOG_WARN(LOGTAG_GRAPHICS, "VK layer '%s' is not supported!", *it);
                it = layers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void GraphicsDevice::ValidateInstanceExtensions(List<const char*>& extensions)
    {
        if (extensions.empty())
        {
            return;
        }

        uint32_t extension_count;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        List<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

        auto it = extensions.begin();
        while (it != extensions.end())
        {
            bool extension_found = false;
            for (const auto& extension_properties : available_extensions)
            {
                if (strcmp(*it, extension_properties.extensionName) == 0)
                {
                    extension_found = true;
                    break;
                }
            }

            if (!extension_found)
            {
                RB_LOG_WARN(LOGTAG_GRAPHICS, "VK instance extension '%s' is not supported!", *it);
                it = extensions.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void GraphicsDevice::ValidateDeviceExtensions(List<const char*>& extensions)
    {
        if (extensions.empty())
        {
            return;
        }

        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extension_count, nullptr);
        List<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice,nullptr, &extension_count, available_extensions.data());

        auto it = extensions.begin();
        while (it != extensions.end())
        {
            bool extension_found = false;
            for (const auto& extension_properties : available_extensions)
            {
                if (strcmp(*it, extension_properties.extensionName) == 0)
                {
                    extension_found = true;
                    break;
                }
            }

            if (!extension_found)
            {
                RB_LOG_WARN(LOGTAG_GRAPHICS, "VK device extension '%s' is not supported!", *it);
                it = extensions.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
#endif