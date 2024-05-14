#include "RabBitCommon.h"
#include "GpuResource.h"
#include "ResourceManager.h"

namespace RB::Graphics::D3D12
{
	GpuResource::GpuResource()
		: m_Resource(nullptr)
		, m_OwnsResource(true)
	{
	}

	GpuResource::GpuResource(GPtr<ID3D12Resource> resource, bool transfer_ownership)
		: m_Resource(resource)
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
		if (IsValid())
		{
			return m_Resource;
		}

		if (!g_ResourceManager->WaitUntilResourceValid(this))
		{
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Resource failed to get valid");
			return nullptr;
		}

		return m_Resource;
	}

	bool GpuResource::IsValid() const
	{
		return m_Resource != nullptr;
	}

	void GpuResource::SetResource(GPtr<ID3D12Resource> resource)
	{
		m_Resource = resource;
	}
}