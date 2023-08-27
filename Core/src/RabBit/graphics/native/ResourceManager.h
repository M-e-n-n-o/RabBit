#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::Native
{
	/*
	* TODO: 
	* - Track the resource sizes
	* - Make is possible to easily create and alias (multiple) resources from 1 heap (all rendertargets in 1 big heap)
	* - Make sure, there is a way to detect which resources should be deleted, and delete them (also remove from ResourceStateManager) 
		(keep all vertex data in memory, stream textures and shader inputs)
	*/

	// Global resource manager
	class ResourceManager
	{
	public:
		ResourceManager();

		GPtr<ID3D12Resource> CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT,
			D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATES start_state = D3D12_RESOURCE_STATE_COMMON, const D3D12_CLEAR_VALUE* clear_value = nullptr);
	};

	extern ResourceManager* g_ResourceManager;
}