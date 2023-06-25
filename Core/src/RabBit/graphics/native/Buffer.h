#pragma once

#include "utils/Ptr.h"
#include "Resource.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::Native
{
	class Texture : public Resource
	{

	};

	class Buffer : public Resource
	{
	};

	// Helper class to wrap command list calls
	class VertexBuffer : public Buffer
	{
		D3D12_VERTEX_BUFFER_VIEW;
	};
}