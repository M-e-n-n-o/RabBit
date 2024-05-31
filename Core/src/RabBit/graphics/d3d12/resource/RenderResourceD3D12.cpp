#include "RabBitCommon.h"
#include "RenderResourceD3D12.h"
#include "graphics/d3d12/UtilsD3D12.h"
#include "ResourceManager.h"

namespace RB::Graphics::D3D12
{
	VertexBufferD3D12::VertexBufferD3D12(const char* name, const TopologyType& type, uint32_t vertex_count_per_instance, void* data, uint64_t data_size)
		: m_Name(name)
		, m_Type(type)
		, m_VertexCountPerInstance(vertex_count_per_instance)
		, m_Size(data_size)
		, m_Data(data)
	{
		m_Resource = new GpuResource();
		g_ResourceManager->ScheduleCreateVertexResource(m_Resource, name, data_size);
	}

	VertexBufferD3D12::~VertexBufferD3D12()
	{
		SAFE_DELETE(m_Resource);
	}

	Texture2DD3D12::Texture2DD3D12(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height)
		: m_Name(name)
		, m_Resource((GpuResource*) internal_resource)
		, m_Width(width)
		, m_Height(height)
		, m_Format(format)
	{

	}
	
	Texture2DD3D12::~Texture2DD3D12()
	{
		SAFE_DELETE(m_Resource);
	}
}