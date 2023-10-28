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

		void InsertResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier);

		void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES to_state, uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
        // Not recommended to use in production
		void TransitionResourceDirect(ID3D12GraphicsCommandList* command_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES to_state, uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

		void InsertUAVBarrier(ID3D12Resource* resource);
		void InsertAliasBarrier(ID3D12Resource* before, ID3D12Resource* after);

		void FlushPendingTransitions(ID3D12GraphicsCommandList* command_list);

	private:
        bool GetCurrentState(ID3D12Resource* resource, uint32_t subresource, D3D12_RESOURCE_STATES& out_state);

        void UpdateResourceState(ID3D12Resource* resource, uint32_t subresource, D3D12_RESOURCE_STATES state);

		struct ResourceState
		{
            // Initialize all of the subresources within a resource to the given state.
            explicit ResourceState(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
                : state(state)
            {}

            // Set a subresource to a particular state.
            void SetSubresourceState(uint32_t subresource, D3D12_RESOURCE_STATES new_state)
            {
                if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
                {
                    state = new_state;
                    subresourceState.clear();
                }
                else
                {
                    subresourceState[subresource] = new_state;
                }
            }

            // Get the state of a (sub)resource within the resource.
            // If the specified subresource is not found in the SubresourceState array (map)
            // then the state of the resource (D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) is
            // returned.
            D3D12_RESOURCE_STATES GetSubresourceState(uint32_t subresource) const
            {
                D3D12_RESOURCE_STATES found_state = state;

                const auto iter = subresourceState.find(subresource);
                if (iter != subresourceState.end())
                {
                    found_state = iter->second;
                }

                return found_state;
            }

            // If the "subresourceState" array (map) is empty, then the "state" variable defines 
            // the state of all of the subresources.
            D3D12_RESOURCE_STATES state;
            Map<UINT, D3D12_RESOURCE_STATES> subresourceState;
		};

        UnorderedMap<ID3D12Resource*, ResourceState> m_ResourceStates;

        List<D3D12_RESOURCE_BARRIER> m_PendingBarriers;
	};

	extern ResourceStateManager* g_ResourceStateManager;
}