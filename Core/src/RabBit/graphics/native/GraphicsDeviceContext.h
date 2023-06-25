#pragma once

#include "utils/Ptr.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::Native
{
	// Helper class to wrap command list calls
	class GraphicsDeviceContext
	{
	public:
		GraphicsDeviceContext();
		~GraphicsDeviceContext();

		void TransitionBarrier()
	};
}