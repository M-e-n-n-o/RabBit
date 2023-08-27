#include "RabBitCommon.h"
#include "ResourceStateManager.h"
#include "GraphicsDevice.h"
#include "DeviceEngine.h"

namespace RB::Graphics::Native
{
	ResourceStateManager* g_ResourceStateManager = nullptr;

	ResourceStateManager::ResourceStateManager()
	{
	}
	
	void ResourceStateManager::StartTrackingResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
	{	
		bool succes = m_ResourceStates.insert({ resource, ResourceState(state) }).second;

		if (!succes)
		{
			RB_LOG_WARN(LOGTAG_GRAPHICS, "Could not start tracking the resource state, maybe this resource is already being tracked?");
		}
	}

	void ResourceStateManager::StopTrackingResource(ID3D12Resource* resource)
	{
		m_ResourceStates.erase(resource);
	}

	void ResourceStateManager::InsertResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier)
	{
		m_PendingBarriers.push_back(barrier);

		if (barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
		{
			UpdateResourceState(barrier.Transition.pResource, barrier.Transition.Subresource, barrier.Transition.StateAfter);
		}
	}

	void ResourceStateManager::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES to_state, uint32_t subresource)
	{
		D3D12_RESOURCE_STATES from_state;
		bool found = GetCurrentState(resource, subresource, from_state);

		if (!found)
		{
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Could not find the current state of the resource, possibly as it is not being tracked yet");
			return;
		}

		InsertResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(resource, from_state, to_state, subresource));
	}

	void ResourceStateManager::TransitionResourceDirect(ID3D12GraphicsCommandList* command_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES to_state, uint32_t subresource)
	{
		D3D12_RESOURCE_STATES from_state;
		bool found = GetCurrentState(resource, subresource, from_state);

		if (!found)
		{
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Could not find the current state of the resource, possibly as it is not being tracked yet");
			return;
		}

		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, from_state, to_state, subresource);
		command_list->ResourceBarrier(1, &barrier);

		UpdateResourceState(resource, subresource, to_state);
	}

	void ResourceStateManager::InsertUAVBarrier(ID3D12Resource* resource)
	{
		InsertResourceBarrier(CD3DX12_RESOURCE_BARRIER::UAV(resource));
	}

	void ResourceStateManager::InsertAliasBarrier(ID3D12Resource* before, ID3D12Resource* after)
	{
		InsertResourceBarrier(CD3DX12_RESOURCE_BARRIER::Aliasing(before, after));
	}

	void ResourceStateManager::FlushPendingTransitions(ID3D12GraphicsCommandList* command_list)
	{
		command_list->ResourceBarrier(m_PendingBarriers.size(), m_PendingBarriers.data());

		m_PendingBarriers.clear();
	}

	bool ResourceStateManager::GetCurrentState(ID3D12Resource* resource, uint32_t subresource, D3D12_RESOURCE_STATES& out_state)
	{
		const auto itr = m_ResourceStates.find(resource);
		
		if (itr == m_ResourceStates.end())
		{
			out_state = D3D12_RESOURCE_STATE_COMMON;
			return false;
		}

		out_state = itr->second.GetSubresourceState(subresource);
		return true;
	}

	void ResourceStateManager::UpdateResourceState(ID3D12Resource* resource, uint32_t subresource, D3D12_RESOURCE_STATES state)
	{
		const auto itr = m_ResourceStates.find(resource);

		if (itr == m_ResourceStates.end())
		{
			return;
		}

		itr->second.SetSubresourceState(subresource, state);
	}
}