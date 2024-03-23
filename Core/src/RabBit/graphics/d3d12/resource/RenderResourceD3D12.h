#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class GpuResource;

	class VertexBufferD3D12 : public VertexBuffer
	{
	public:
		VertexBufferD3D12(const char* name, void* data, uint64_t data_size);
		~VertexBufferD3D12();

		const char* GetName() const override { return m_Name; }
		
		virtual void* GetData() const override { return m_Data; }

		void* GetNativeResource() const override { return m_Resource; }

		uint32_t GetSize() const override { return m_Size; }

		D3D12_VERTEX_BUFFER_VIEW GetView() const { return m_View; }

	private:
		const char*					m_Name;
		GpuResource*				m_Resource;
		D3D12_VERTEX_BUFFER_VIEW	m_View;
		uint32_t					m_Size;
		void*						m_Data;
	};
}