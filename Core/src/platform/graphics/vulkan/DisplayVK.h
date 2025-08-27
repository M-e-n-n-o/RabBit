#if RB_GRAPHICS_API_VULKAN

#pragma once
#include "graphics/Display.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class DisplayVK : public Display
    {
    public:
        DisplayVK();

        const char* GetName() override { return m_Name; }

        Math::Float2 GetResolution() override { return m_Resolution; }

    private:
        const char*         m_Name;
        RB::Math::Float2    m_Resolution;
    };

    List<Display*> CreateDisplays();
}
#endif