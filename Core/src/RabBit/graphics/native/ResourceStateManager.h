#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::Native
{
	class ResourceStateManager
	{
	public:
		ResourceStateManager();

		void StartTrackingResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);

		void StopTrackingResource(ID3D12Resource* resource);
	};

	extern ResourceStateManager* g_ResourceStateManager;
}