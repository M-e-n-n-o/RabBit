#if RB_PLATFORM_LINUX_ES

#include "RabBitCommon.h"
#include "events/input/Input.h"

namespace RB::Events
{
    bool IsKeyDown(const KeyCode& key)
    {
        return false;
    }

    bool IsMouseKeyDown(const MouseCode& mouse_button)
    {
        return false;
    }

    Math::Float2 GetMousePos()
    {
        return Math::Float2(0);
    }
}
#endif