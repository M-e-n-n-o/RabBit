#include "RabBitCommon.h"
#include "RenderResourceD3D12.h"
#include "ResourceManager.h"
#include <D3DX12/d3dx12.h>

namespace RB::Graphics::D3D12
{
	VertexBufferD3D12::VertexBufferD3D12(const char* name, void* data, uint64_t data_size)
		: m_Name(name)
		, m_Size(data_size)
		, m_Data(data)
	{
		m_Resource = new GpuResource();
	}

	VertexBufferD3D12::~VertexBufferD3D12()
	{
		SAFE_DELETE(m_Resource);
	}
}