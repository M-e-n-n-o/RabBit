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
		g_ResourceManager->MarkForDelete(this);
	}

	GPtr<ID3D12Resource> GpuResource::GetResource()
	{
		// TODO when resources are created on a separate thread
		// - Tell the resource creation thread something is waiting on the resource so it gets a higher priority 
		// - Waits until the actual resource is created and available

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