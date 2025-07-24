#include "RabBitCommon.h"
#include "ResourceStateManager.h"
#include "GpuResource.h"
#include "../GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
    ResourceStateManager* g_ResourceStateManager = nullptr;

    ResourceStateManager::ResourceStateManager()
    {
    }

    void ResourceStateManager::InsertResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier)
    {
        m_PendingBarriers.push_back(barrier);
    }

    void ResourceStateManager::TransitionResource(GpuResource* resource, D3D12_RESOURCE_STATES to_state)
    {
        if (resource->IsInState(to_state))
        {
            return;
        }

        ID3D12Resource* res = resource->GetResource().Get();

        D3D12_RESOURCE_STATES old_state = resource->GetState();

        resource->UpdateState(to_state);

        InsertResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(res, old_state, to_state, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
    }

    void ResourceStateManager::InsertUAVBarrier(GpuResource* resource)
    {
        ID3D12Resource* res = resource ? resource->GetResource().Get() : nullptr;
        InsertResourceBarrier(CD3DX12_RESOURCE_BARRIER::UAV(res));
    }

    void ResourceStateManager::InsertAliasBarrier(GpuResource* before, GpuResource* after)
    {
        InsertResourceBarrier(CD3DX12_RESOURCE_BARRIER::Aliasing(before->GetResource().Get(), after->GetResource().Get()));
    }

    void ResourceStateManager::FlushPendingTransitions(ID3D12GraphicsCommandList* command_list)
    {
        if (m_PendingBarriers.empty())
        {
            return;
        }

        command_list->ResourceBarrier(m_PendingBarriers.size(), m_PendingBarriers.data());

        m_PendingBarriers.clear();
    }
}