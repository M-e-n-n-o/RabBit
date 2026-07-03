#if RB_GRAPHICS_API_D3D12

#include "RabBitCommon.h"
#include "GpuResource.h"
#include "ResourceManager.h"
#include "GpuResource.h"

namespace RB::Graphics::D3D12
{
    GpuResource::GpuResource(std::function<void(GpuResource*)> on_resource_created_callback)
        : m_Resource(nullptr)
        , m_State(D3D12_RESOURCE_STATE_COMMON)
        , m_OwnsResource(true)
        , m_IsValid(false)
        , m_OnCreationCallback(on_resource_created_callback)
    {
    }

    GpuResource::GpuResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state, bool transfer_ownership)
        : m_Resource(resource)
        , m_State(state)
        , m_OwnsResource(transfer_ownership)
        , m_IsValid(true)
        , m_OnCreationCallback(nullptr)
    {
    }

    GpuResource::~GpuResource()
    {
        if (m_OwnsResource)
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
        return m_IsValid;
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

        if (!g_ResourceManager->WaitUntilResourceValid(this))
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Resource failed to get valid");
        }

        RB_ASSERT(LOGTAG_GRAPHICS, m_Resource != nullptr, "Resource somehow still not valid");
    }

    void GpuResource::SetResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
    {
        m_Resource = resource;
        m_State = state;

        if (m_OnCreationCallback)
        {
            m_OnCreationCallback(this);
        }

        m_IsValid = true;
    }
}
#endif