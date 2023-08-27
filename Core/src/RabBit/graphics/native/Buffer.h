#pragma once

#include "RabBitCommon.h"
#include "Resource.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::Native
{
	class Buffer : public Resource
	{
	public:
		Buffer(const D3D12_RESOURCE_DESC& resource_desc, uint32_t num_elements, uint32_t element_size, void* data, const wchar_t* name = L"");
		virtual ~Buffer();

		const void*	GetData()			const { return m_Data; }
		uint32_t	GetNumElements()	const { return m_NumElements; }
		uint32_t	GetElementSize()	const { return m_ElementSize; }
		uint32_t	GetBufferSize()		const { return m_NumElements * m_ElementSize; }

	private:
		virtual void CreateViews(uint32_t num_elements, uint32_t element_size) = 0;

		const void*		m_Data;
		const uint32_t	m_NumElements;
		const uint32_t	m_ElementSize;
	};

	// Buffer of which the data can be changed
	//class DynamicBuffer : public Buffer
	//{
	//public:
	//	// Leave the element parameters at 0 if the size of the buffer stays the same
	//	void SetData(void* data, uint32_t num_elements = 0, uint32_t element_size = 0);
	//};

	class VertexBuffer : public Buffer
	{
	public:
		VertexBuffer(uint32_t num_elements, uint32_t element_size, void* data, const wchar_t* name = L"");

	private:
		void CreateViews(uint32_t num_elements, uint32_t element_size) override;

		D3D12_VERTEX_BUFFER_VIEW m_VertexView;
	};
}