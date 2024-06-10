#pragma once

#include "Frustum.h"

namespace RB::Graphics
{
	class RenderInterface;

	class ViewContext
	{
	public:
		Frustum		viewFrustum;
		uint32_t	displayIndex;

		void SetFrameConstants(RenderInterface* render_interface) const;
	};
}