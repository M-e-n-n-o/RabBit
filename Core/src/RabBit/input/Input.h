#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"

namespace RB::Input
{
	bool IsKeyDown(const KeyCode& key);

	bool IsMouseKeyDown(const MouseCode& mouse_button);
}