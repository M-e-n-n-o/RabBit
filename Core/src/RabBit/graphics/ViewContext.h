#pragma once

#include "Frustum.h"

namespace RB::Graphics
{
	struct ViewContext
	{
		Frustum		viewFrustum;
		uint32_t	displayIndex;
	};
}