#include "RabBitPch.h"
#include "ResourceStateManager.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::Native
{
	ResourceStateManager* g_ResourceStateManager = nullptr;

	ResourceStateManager::ResourceStateManager()
	{
	}
	
	void ResourceStateManager::StartTrackingResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
	{
		// TODO
	}

	void ResourceStateManager::StopTrackingResource(ID3D12Resource* resource)
	{
		// TODO
	}
}