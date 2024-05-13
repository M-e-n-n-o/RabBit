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
		VertexBufferD3D12(const char* name, const TopologyType& type, void* data, uint64_t data_size);
		~VertexBufferD3D12();

		const char* GetName() const override { return m_Name; }
		
		void* GetData() const override { return m_Data; }

		void* GetNativeResource() const override { return m_Resource; }

		uint32_t GetSize() const override { return m_Size; }

		TopologyType GetTopologyType() const override { return m_Type; }

		D3D12_VERTEX_BUFFER_VIEW GetView() const { return m_View; }

	private:
		const char*					m_Name;
		GpuResource*				m_Resource;
		D3D12_VERTEX_BUFFER_VIEW	m_View;
		TopologyType				m_Type;
		uint32_t					m_Size;
		void*						m_Data;
	};

	class Texture2DD3D12 : public Texture2D
	{
	public:
		Texture2DD3D12(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height);
		~Texture2DD3D12();

		const char* GetName() const override { return m_Name; }

		void* GetNativeResource() const override { return m_Resource; }

		RenderResourceFormat GetFormat() const override { return m_Format; }

		void* GetData() const override { return nullptr; }

		uint32_t GetSize() const override { return -1; }

		bool AllowedRenderTarget() const override { return false; }

		uint32_t GetWidth() const override { return m_Width; }
		uint32_t GetHeight() const override { return m_Height; }

	private:
		const char*				m_Name;
		GpuResource*			m_Resource;
		uint32_t				m_Width;
		uint32_t				m_Height;
		RenderResourceFormat	m_Format;
	};
}