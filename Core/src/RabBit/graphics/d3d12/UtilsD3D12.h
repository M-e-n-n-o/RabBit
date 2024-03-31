#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	static D3D_PRIMITIVE_TOPOLOGY ConvertToD3D12Topology(const TopologyType& type)
	{
		switch (type)
		{
		case TopologyType::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		default:
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}
	}
}