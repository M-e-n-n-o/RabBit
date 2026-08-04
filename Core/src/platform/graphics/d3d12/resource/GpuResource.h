#if RB_GRAPHICS_API_D3D12

#pragma once

#include "RabBitCommon.h"

#include <d3d12.h>

namespace RB::Graphics
{
    class GpuGuard;
}

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

        void SetResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state, const Shared<GpuGuard>& upload_guard);

        bool IsValid() const;
        bool IsDataUploadComplete(bool block_until_ready = false);

        void UpdateState(D3D12_RESOURCE_STATES state);
        D3D12_RESOURCE_STATES GetState() const;
        bool IsInState(D3D12_RESOURCE_STATES state) const;

        void AwaitValidation() const;

    private:
        inline bool CheckFlag(uint8_t flag) const;
        inline void SetFlag(uint8_t flag, bool value);

        enum Flags : uint8_t
        {
            kOwnsResource   = 1 << 0,
            kIsValid        = 1 << 1
        };

        ID3D12Resource*                     m_Resource;
        D3D12_RESOURCE_STATES               m_State;
        std::function<void(GpuResource*)>   m_OnCreationCallback;
        Shared<GpuGuard>                    m_UploadGuard;
        uint8_t                             m_Flags;
    };
}
#endif