#include "RabBitCommon.h"
#include "events/input/Input.h"

#include <winuser.h>

namespace RB::Events
{
    bool IsKeyDown(const KeyCode& key)
    {
        SHORT state = GetAsyncKeyState(static_cast<int>(key));

        // Check high bit
        return (1 << 15) & state;
    }

    bool IsMouseKeyDown(const MouseCode& mouse_button)
    {
        SHORT state = GetAsyncKeyState(static_cast<int>(mouse_button));

        // Check high bit
        return (1 << 15) & state;
    }

    Math::Float2 GetMousePos()
    {
        POINT point;
        if (!GetCursorPos(&point))
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "GetCursorPos failed.Error: %ws", GetLastError());
        }

        return Math::Float2(point.x, point.y);
    }
}