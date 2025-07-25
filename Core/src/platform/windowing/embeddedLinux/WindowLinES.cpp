#if RB_PLATFORM_LINUX_ES

#include "RabBitCommon.h"
#include "WindowLinES.h"
#include "app/Application.h"
#include "graphics/Display.h"
#include "graphics/Renderer.h"
#include "events/WindowEvent.h"

using namespace RB::Events;

namespace RB::Graphics::LinuxES
{
    WindowLinuxES::WindowLinuxES(const WindowArgs args)
        : Window(true, 1.0f, 0.0f)
    {

    }
}
#endif