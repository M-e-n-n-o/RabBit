#include "RabBitCommon.h"
#include "Buffer.h"

#include <D3DX12/d3dx12.h>

namespace RB::Graphics::Native
{
	// --------------------------------------------------------
	//                        Buffer
	// --------------------------------------------------------

	Buffer::Buffer(const D3D12_RESOURCE_DESC& resource_desc, uint32_t num_elements, uint32_t element_size, void* data, const wchar_t* name)
		: Resource(resource_desc, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, name)
		, m_Data(data)
		, m_NumElements(num_elements)
		, m_ElementSize(element_size)
	{
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, resource_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER, "A buffer should always have a buffer dimension");

		CreateViews();
	}

	Buffer::~Buffer()
	{
	}

	// --------------------------------------------------------
	//                    Vertex Buffer
	// --------------------------------------------------------

	VertexBuffer::VertexBuffer(uint32_t num_elements, uint32_t element_size, void* vertex_data, const wchar_t* name)
		: Buffer(CD3DX12_RESOURCE_DESC::Buffer(num_elements * element_size), num_elements, element_size, vertex_data, name)
	{
	}

	void VertexBuffer::CreateViews()
	{
		m_VertexView = {};
		m_VertexView.BufferLocation = m_Resource->GetGPUVirtualAddress();
		m_VertexView.SizeInBytes	= GetBufferSize();
		m_VertexView.StrideInBytes	= m_NumElements;
	}

	// --------------------------------------------------------
	//                    Index Buffer
	// --------------------------------------------------------

	#define CALC_INDEX_ELEM_SIZE(use_32_bit_precision) ((use_32_bit_precision) ? 4 : 2)

	IndexBuffer::IndexBuffer(uint32_t num_indices, bool use_32_bit_precision, void* indices, const wchar_t* name)
		: Buffer(CD3DX12_RESOURCE_DESC::Buffer(num_indices * CALC_INDEX_ELEM_SIZE(use_32_bit_precision)), num_indices, CALC_INDEX_ELEM_SIZE(use_32_bit_precision), indices, name)
		, m_Use32BitPrecision(use_32_bit_precision)
	{
	}

	void IndexBuffer::CreateViews()
	{
		m_IndexView = {};
		m_IndexView.BufferLocation	= m_Resource->GetGPUVirtualAddress();
		m_IndexView.SizeInBytes		= GetBufferSize();
		m_IndexView.Format			= m_Use32BitPrecision ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	}
}