#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "DeviceQueue.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::VK
{
    DeviceQueue::DeviceQueueVK(VkQueueFlags type, uint32_t queue_family_index, uint32_t queue_index)
    {
        // Create the device queue here!
        static_assert(false);
    }

    DeviceQueue::~DeviceQueueVK()
    {
    }
}
#endif