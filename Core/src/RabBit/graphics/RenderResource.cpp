#include "RabBitCommon.h"
#include "RenderResource.h"
#include "Renderer.h"
#include "d3d12/resource/RenderResourceD3D12.h"

namespace RB::Graphics
{
	RenderResourceType RenderResource::GetPrimitiveType() const
	{
		uint32_t last_primitive = (uint32_t)RenderResourceType::kLastPrimitiveType;
		uint32_t and_value = last_primitive + (last_primitive - 1);

		uint32_t primitive_type = (uint32_t)m_Type & and_value;

		return (RenderResourceType) primitive_type;
	}

	VertexBuffer* VertexBuffer::Create(const char* name, const TopologyType& type, void* data, uint64_t data_size)
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
			return new D3D12::VertexBufferD3D12(name, type, data, data_size);
		default:
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}

		return nullptr;
	}
	
	Texture2D* Texture2D::Create(const char* name, void* internal_resource, uint32_t width, uint32_t height)
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
			return new D3D12::Texture2DD3D12(name, internal_resource, width, height);
		default:
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}

		return nullptr;
	}
}