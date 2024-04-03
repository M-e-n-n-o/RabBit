#include "RabBitCommon.h"
#include "RenderResourceD3D12.h"
#include "ResourceManager.h"

namespace RB::Graphics::D3D12
{
	VertexBufferD3D12::VertexBufferD3D12(const char* name, const TopologyType& type, void* data, uint64_t data_size)
		: m_Name(name)
		, m_Type(type)
		, m_Size(data_size)
		, m_Data(data)
	{
		m_Resource = new GpuResource();
	}

	VertexBufferD3D12::~VertexBufferD3D12()
	{
		SAFE_DELETE(m_Resource);
	}

	Texture2DD3D12::Texture2DD3D12(const char* name, void* internal_resource, uint32_t width, uint32_t height)
		: m_Name(name)
		, m_Resource((GpuResource*) internal_resource)
		, m_Width(width)
		, m_Height(height)
	{

	}
	
	Texture2DD3D12::~Texture2DD3D12()
	{
		SAFE_DELETE(m_Resource);
	}
}