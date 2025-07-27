#if RB_GRAPHICS_API_VULKAN

#pragma once
#include "RabBitCommon.h"
#include "graphics/Renderer.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class GraphicsDevice
    {
    };

    extern GraphicsDevice* g_GraphicsDevice;
}
#endif