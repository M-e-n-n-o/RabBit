#include "RabBitCommon.h"
#include "Input.h"

#include <winuser.h>

namespace RB::Input
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
}