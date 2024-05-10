#include "RabBitCommon.h"
#include "GpuResource.h"
#include "ResourceManager.h"

namespace RB::Graphics::D3D12
{
	GpuResource::GpuResource()
		: m_Resource(nullptr)
	{
	}

	GpuResource::~GpuResource()
	{
		if (IsValid())
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