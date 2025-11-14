#if RB_GRAPHICS_API_D3D12

#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    class DeviceQueue;

    class GpuResource
    {
    public:
        GpuResource(std::function<void(GpuResource*)> on_resource_created_callback = nullptr);
        GpuResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state, bool transfer_ownership);
        ~GpuResource();

        // Should only be called on the render thread!
        ID3D12Resource* GetResource();

        void SetResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);

        bool IsValid() const;

        void UpdateState(D3D12_RESOURCE_STATES state);
        D3D12_RESOURCE_STATES GetState() const;
        bool IsInState(D3D12_RESOURCE_STATES state) const;

    private:
        void AwaitValidation() const;

        ID3D12Resource*                     m_Resource;
        D3D12_RESOURCE_STATES               m_State;
        bool                                m_OwnsResource;
        bool                                m_IsValid;
        std::function<void(GpuResource*)>   m_OnCreationCallback;
    };
}
#endif