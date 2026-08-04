#if RB_GRAPHICS_API_D3D12

#include "RabBitCommon.h"
#include "GpuResource.h"
#include "ResourceManager.h"
#include "graphics/RenderInterface.h"

namespace RB::Graphics::D3D12
{
    GpuResource::GpuResource(std::function<void(GpuResource*)> on_resource_created_callback)
        : m_Resource(nullptr)
        , m_State(D3D12_RESOURCE_STATE_COMMON)
        , m_OnCreationCallback(on_resource_created_callback)
        , m_UploadGuard(nullptr)
    {
        SetFlag(kOwnsResource, true);
        SetFlag(kIsValid, false);
    }

    GpuResource::GpuResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state, bool transfer_ownership)
        : m_Resource(resource)
        , m_State(state)
        , m_OnCreationCallback(nullptr)
        , m_UploadGuard(nullptr)
    {
        SetFlag(kOwnsResource, transfer_ownership);
        SetFlag(kIsValid, true);
    }

    GpuResource::~GpuResource()
    {
        if (CheckFlag(kOwnsResource))
        {
            g_ResourceManager->MarkForDelete(this);
        }
    }

    ID3D12Resource* GpuResource::GetResource()
    {
        AwaitValidation();
        return m_Resource;
    }

    bool GpuResource::IsValid() const
    {
        return CheckFlag(kIsValid);
    }

    bool GpuResource::IsDataUploadComplete(bool block_until_ready)
    {
        if (m_UploadGuard)
        {
            bool ready;
            if (block_until_ready)
            {
                m_UploadGuard->WaitUntilFinishedRendering();
                ready = true;
            }
            else
                ready = m_UploadGuard->IsFinishedRendering();

            if (ready)
                m_UploadGuard = nullptr;

            return ready;
        }
        return true;
    }

    void GpuResource::UpdateState(D3D12_RESOURCE_STATES state)
    {
        AwaitValidation();
        m_State = state;
    }

    D3D12_RESOURCE_STATES GpuResource::GetState() const
    {
        AwaitValidation();
        return m_State;
    }

    bool GpuResource::IsInState(D3D12_RESOURCE_STATES state) const
    {
        AwaitValidation();
        return m_State == state;
    }

    void GpuResource::AwaitValidation() const
    {
        if (IsValid())
        {
            return;
        }

        if (!g_ResourceManager->WaitUntilResourceCreated(this))
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Resource failed to get valid");
        }

        RB_ASSERT(LOGTAG_GRAPHICS, m_Resource != nullptr, "Resource somehow still not valid");
    }

    bool GpuResource::CheckFlag(uint8_t flag) const
    {
        return (m_Flags & flag) > 0;
    }

    void GpuResource::SetFlag(uint8_t flag, bool value)
    {
        if (value)
            m_Flags |= flag;
        else
            m_Flags &= ~flag;
    }

    void GpuResource::SetResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state, const Shared<GpuGuard>& upload_guard)
    {
        m_Resource    = resource;
        m_State       = state;
        m_UploadGuard = upload_guard;

        if (m_OnCreationCallback)
        {
            m_OnCreationCallback(this);
        }

        SetFlag(kIsValid, true);
    }
}
#endif