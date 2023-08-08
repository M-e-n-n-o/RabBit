#include "RabBitCommon.h"
#include "ResourceManager.h"
#include "GraphicsDevice.h"
#include "ResourceStateManager.h"

namespace RB::Graphics::Native
{
	ResourceManager* g_ResourceManager = nullptr;

	ResourceManager::ResourceManager()
	{
	}

	GPtr<ID3D12Resource> ResourceManager::CreateCommittedResource(const wchar_t* name, const D3D12_RESOURCE_DESC& resource_desc, D3D12_HEAP_TYPE heap_type,
		D3D12_HEAP_FLAGS heap_flags, D3D12_RESOURCE_STATES start_state, const D3D12_CLEAR_VALUE* clear_value)
	{
		GPtr<ID3D12Resource> resource = nullptr;

		RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(heap_type),
			heap_flags,
			&resource_desc,
			start_state,
			clear_value,
			IID_PPV_ARGS(&resource)
		), "Could not create committed resource: %s", name);

		if (resource)
		{
			resource->SetName(name);

			// Start tracking resources state
			g_ResourceStateManager->StartTrackingResource(resource.Get(), start_state);
		}

		return resource;
	}
}