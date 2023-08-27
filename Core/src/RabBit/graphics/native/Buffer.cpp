#include "RabBitCommon.h"
#include "Buffer.h"

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
		CreateViews(num_elements, element_size);
	}

	Buffer::~Buffer()
	{
	}

	// --------------------------------------------------------
	//                    Vertex Buffer
	// --------------------------------------------------------

	//VertexBuffer::VertexBuffer(uint32_t num_elements, uint32_t element_size, void* data, const wchar_t* name)
	//	: Buffer()
	//{
	//}

	//void VertexBuffer::CreateViews(uint32_t num_elements, uint32_t element_size)
	//{
	//}
} 