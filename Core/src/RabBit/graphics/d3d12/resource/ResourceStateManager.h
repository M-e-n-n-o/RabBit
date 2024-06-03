#pragma once

#include "RabBitCommon.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    class GpuResource;

	// TODO Make this class thread safe
	class ResourceStateManager
	{
	public:
		ResourceStateManager();

		void TransitionResource(GpuResource* resource, D3D12_RESOURCE_STATES to_state);

		void InsertUAVBarrier(GpuResource* resource);
		void InsertAliasBarrier(GpuResource* before, GpuResource* after);

		void FlushPendingTransitions(ID3D12GraphicsCommandList* command_list);

	private:
        void InsertResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier);

        List<D3D12_RESOURCE_BARRIER> m_PendingBarriers;
	};

	extern ResourceStateManager* g_ResourceStateManager;
}