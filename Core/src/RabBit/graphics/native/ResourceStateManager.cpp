#include "RabBitCommon.h"
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

	void ResourceStateManager::InsertResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier)
	{
	}

	void ResourceStateManager::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES to_state, uint32_t subresource)
	{
	}

	void ResourceStateManager::TransitionResourceDirect(ID3D12Resource* resource, D3D12_RESOURCE_STATES to_state, uint32_t subresource)
	{
	}

	void ResourceStateManager::InsertUAVBarrier(ID3D12Resource* resource)
	{
	}

	void ResourceStateManager::InsertAliasBarrier(ID3D12Resource* before, ID3D12Resource* after)
	{
	}

	void ResourceStateManager::FlushPendingTransitions()
	{
	}
}