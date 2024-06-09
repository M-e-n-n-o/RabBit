#include "RabBitCommon.h"
#include "RenderResourceD3D12.h"
#include "graphics/d3d12/UtilsD3D12.h"
#include "ResourceManager.h"

namespace RB::Graphics::D3D12
{
	VertexBufferD3D12::VertexBufferD3D12(const char* name, const TopologyType& type, void* data, uint32_t vertex_size, uint64_t data_size)
		: m_Name(name)
		, m_Type(type)
		, m_VertexSize(vertex_size)
		, m_Size(data_size)
		, m_Data(data)
		, m_View{}
	{
		m_Resource = new GpuResource();
		g_ResourceManager->ScheduleCreateVertexResource(m_Resource, name, data_size);
	}

	VertexBufferD3D12::~VertexBufferD3D12()
	{
		SAFE_DELETE(m_Resource);
	}

	D3D12_VERTEX_BUFFER_VIEW VertexBufferD3D12::GetView()
	{
		if (m_View.SizeInBytes == 0)
		{
			m_View.BufferLocation = m_Resource->GetResource()->GetGPUVirtualAddress();
			m_View.SizeInBytes	  = m_Size;
			m_View.StrideInBytes  = m_VertexSize;
		}

		return m_View;
	}

	IndexBufferD3D12::IndexBufferD3D12(const char* name, uint16_t* data, uint64_t data_size)
		: m_Name(name)
		, m_Size(data_size)
		, m_Data(data)
		, m_View{}
	{
		m_Resource = new GpuResource();
		g_ResourceManager->ScheduleCreateIndexResource(m_Resource, name, data_size);
	}

	IndexBufferD3D12::~IndexBufferD3D12()
	{
		SAFE_DELETE(m_Resource);
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferD3D12::GetView()
	{
		if (m_View.SizeInBytes == 0)
		{
			m_View.BufferLocation = m_Resource->GetResource()->GetGPUVirtualAddress();
			m_View.SizeInBytes	  = m_Size;
			m_View.Format		  = ConvertToDXGIFormat(GetFormat());
		}

		return m_View;
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