#pragma once

#include "Frustum.h"
#include "math/Vector.h"

namespace RB::Graphics
{
	class RenderInterface;
	class Texture2D;

	class ViewContext
	{
	public:
		bool			isOffscreenContext;
		uint32_t		windowIndex;
		Texture2D*		finalColorTarget;
		Math::Float4	clearColor;

		Frustum			viewFrustum;

		void SetFrameConstants(RenderInterface* render_interface) const;
	};
}