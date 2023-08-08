#pragma once

#include "RabBitCommon.h"
#include "Resource.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::Native
{
	// Move this class in a separate file
	class Texture : public Resource
	{

	};

	class Buffer : public Resource
	{
	public:
		Buffer(const D3D12_RESOURCE_DESC& resource_desc, const std::wstring& name = L"");
		virtual ~Buffer();

	private:
		virtual void CreateViews(uint32_t num_elements, uint32_t element_size) = 0;
	};

	// Helper class to wrap command list calls
	class VertexBuffer : public Buffer
	{
		D3D12_VERTEX_BUFFER_VIEW;
	};
}