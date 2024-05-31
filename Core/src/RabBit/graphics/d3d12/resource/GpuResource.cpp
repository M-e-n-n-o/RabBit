#include "RabBitCommon.h"
#include "GpuResource.h"
#include "ResourceManager.h"

namespace RB::Graphics::D3D12
{
	GpuResource::GpuResource()
		: m_Resource(nullptr)
		, m_State(D3D12_RESOURCE_STATE_COMMON)
		, m_OwnsResource(true)
	{
	}

	GpuResource::GpuResource(GPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state, bool transfer_ownership)
		: m_Resource(resource)
		, m_State(state)
		, m_OwnsResource(transfer_ownership)
	{
	}

	GpuResource::~GpuResource()
	{
		if (IsValid() && m_OwnsResource)
		{
			g_ResourceManager->MarkForDelete(this);
		}
	}

	GPtr<ID3D12Resource> GpuResource::GetResource()
	{
		if (!IsValid())
		{
			if (!g_ResourceManager->WaitUntilResourceValid(this))
			{
				RB_LOG_ERROR(LOGTAG_GRAPHICS, "Resource failed to get valid");
				return nullptr;
			}
		}

		return m_Resource;
	}

	bool GpuResource::IsValid() const
	{
		return m_Resource != nullptr;
	}

	void GpuResource::UpdateState(D3D12_RESOURCE_STATES state)
	{
		m_State = state;
	}

	D3D12_RESOURCE_STATES GpuResource::GetState() const
	{
		return m_State;
	}

	bool GpuResource::IsInState(D3D12_RESOURCE_STATES state) const
	{
		if (state == D3D12_RESOURCE_STATE_COMMON)
			return m_State == state;
		else
			return ((int)m_State) & ((int)state) == state;
	}

	void GpuResource::SetResource(GPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state)
	{
		m_Resource = resource;
		m_State = state;
	}
}