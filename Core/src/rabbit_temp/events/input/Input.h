#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"
#include "math/Vector.h"

namespace RB::Events
{
    bool IsKeyDown(const KeyCode& key);

    bool IsMouseKeyDown(const MouseCode& mouse_button);

    Math::Float2 GetMousePos();
}