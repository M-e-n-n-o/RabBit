#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "DisplayVK.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::VK
{
    DisplayVK::DisplayVK()
    {
        // TODO:
        //vkGetPhysicalDeviceDisplayPropertiesKHR
        //vkGetPhysicalDeviceDisplayPlanePropertiesKHR
        //vkGetDisplayPlaneCapabilitiesKHR
        //vkCreateDisplayPlaneSurfaceKHR
        //vkGetPhysicalDeviceSurfaceSupportKHR (check for this in GraphicsDevice.cpp?)
    }

    List<Display*> CreateDisplays()
    {
        List<Display*> displays;

        return displays;
    }
}
#endif