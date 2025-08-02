#include "RabBitCommon.h"
#include "Display.h"
#include "graphics/Renderer.h"

#if RB_PLATFORM_WINDOWS
#include "platform/windowing/windows/DisplayWin.h"
#endif

#if RB_GRAPHICS_API_VULKAN && !RB_PLATFORM_WINDOWS
// Enumerating the displays using the Vulkan API is not supported on Windows
#include "platform/graphics/vulkan/DisplayVK.h"
#endif

namespace RB::Graphics
{
    Display::Display()
    {
    }

    Display::~Display()
    {
    }

    List<Display*> Display::CreateDisplays()
    {
        List<Display*> displays;
#if RB_PLATFORM_WINDOWS
        displays = Windows::CreateDisplays();
#elif RB_GRAPHICS_API_VULKAN
        // TODO
        static_assert(false);
#else
        RB_LOG_CRITICAL(LOGTAG_WINDOWING, "Did not yet implement the display class for the windowing platform");
#endif

        RB_LOG(LOGTAG_WINDOWING, "-------- MONITOR INFORMATION --------");
        RB_LOG(LOGTAG_WINDOWING, "Found the following displays:");

        for (int i = 0; i < displays.size(); ++i)
        {
            RB_LOG(LOGTAG_WINDOWING, "\t%d. %s (%d x %d)", i + 1, displays[i]->GetName(), (int)(displays[i]->GetResolution().x), (int)(displays[i]->GetResolution().y));
        }

        return displays;
    }
}