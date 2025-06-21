#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"

namespace RB::Events
{
    bool IsKeyDown(const KeyCode& key);

    bool IsMouseKeyDown(const MouseCode& mouse_button);
}