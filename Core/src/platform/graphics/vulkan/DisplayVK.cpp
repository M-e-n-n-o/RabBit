#if RB_GRAPHICS_API_VULKAN

#include "RabBitCommon.h"
#include "DisplayVK.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::VK
{
    DisplayVK::DisplayVK(VkDisplayKHR handle, const char name[128], RB::Math::Float2 resolution)
        : m_Handle(handle)
        , m_Resolution(resolution)
    {
        // TODO Add extra checking for the displays using vkGetDisplayPlaneCapabilitiesKHR
        std::snprintf(m_Name, sizeof(m_Name), "%s", name);
    }

    List<Display*> CreateDisplays()
    {
        List<Display*> out_displays;

        uint32_t displayCount = 0;
        vkGetPhysicalDeviceDisplayPropertiesKHR(g_GraphicsDevice->GetPhysicalDevice(), &displayCount, nullptr);
        std::vector<VkDisplayPropertiesKHR> displays(displayCount);
        vkGetPhysicalDeviceDisplayPropertiesKHR(g_GraphicsDevice->GetPhysicalDevice(), &displayCount, displays.data());

        for (uint32_t i = 0; i < displayCount; ++i)
        {
            VkDisplayPropertiesKHR& disp = displays[i];

            std::string name = disp.displayName ? disp.displayName : "Unknown";

            RB::Math::Float2 resolution;
            resolution.x = static_cast<float>(disp.physicalResolution.width);
            resolution.y = static_cast<float>(disp.physicalResolution.height);

            Display* displayObj = new DisplayVK(disp.display, name.c_str(), resolution);
            out_displays.push_back(displayObj);
        }

        return out_displays;
    }
}
#endif