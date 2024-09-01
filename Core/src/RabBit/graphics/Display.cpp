#include "RabBitCommon.h"
#include "Display.h"
#include "graphics/Renderer.h"
#include "graphics/d3d12/window/DisplayD3D12.h"

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

        switch (Renderer::GetAPI())
        {
        case RenderAPI::D3D12: displays = D3D12::CreateDisplays(); break;

        default:
            RB_LOG_CRITICAL(LOGTAG_WINDOWING, "Did not yet implement the display class for the set graphics API");
            break;
        }

        RB_LOG(LOGTAG_WINDOWING, "-------- MONITOR INFORMATION --------");
        RB_LOG(LOGTAG_WINDOWING, "Found the following displays:");

        for (int i = 0; i < displays.size(); ++i)
        {
            RB_LOG(LOGTAG_WINDOWING, "\t%d. %s (%d x %d)", i + 1, displays[i]->GetName(), (int)(displays[i]->GetResolution().x), (int)(displays[i]->GetResolution().y));
        }

        return displays;
    }
}