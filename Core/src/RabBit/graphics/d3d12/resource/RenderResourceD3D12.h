#pragma once

#include "RabBitCommon.h"
#include "Descriptor.h"
#include "graphics/RenderResource.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    class GpuResource;

    class VertexBufferD3D12 : public VertexBuffer
    {
    public:
        VertexBufferD3D12(const char* name, const TopologyType& type, void* data, uint32_t vertex_size, uint64_t data_size);
        ~VertexBufferD3D12();

        const char* GetName() const override { return m_Name; }

        void* GetNativeResource() const override { return m_Resource; }

        uint32_t GetVertexElementCount() const override { return m_Size / m_VertexSize; }

        TopologyType GetTopologyType() const override { return m_Type; }

        D3D12_VERTEX_BUFFER_VIEW GetView();

    private:
        const char*                 m_Name;
        GpuResource*                m_Resource;
        D3D12_VERTEX_BUFFER_VIEW	m_View;
        TopologyType				m_Type;
        uint32_t					m_VertexSize;
        uint64_t					m_Size;
        void*                       m_Data;
    };

    class IndexBufferD3D12 : public IndexBuffer
    {
    public:
        IndexBufferD3D12(const char* name, uint32_t* data, uint64_t data_size);
        ~IndexBufferD3D12();

        const char* GetName() const override { return m_Name; }

        void* GetNativeResource() const override { return m_Resource; }

        uint64_t GetIndexCount() const override { return m_Size / sizeof(uint32_t); }

        D3D12_INDEX_BUFFER_VIEW GetView();

    private:
        const char*                 m_Name;
        GpuResource*                m_Resource;
        D3D12_INDEX_BUFFER_VIEW		m_View;
        uint64_t					m_Size;
        void*                       m_Data;
    };

    class Texture2DD3D12 : public Texture2D
    {
    public:
        Texture2DD3D12(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space);
        Texture2DD3D12(const char* name, void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space);
        Texture2DD3D12(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space);
        ~Texture2DD3D12();

        const char* GetName() const override { return m_Name; }

        void* GetNativeResource() const override { return m_Resource; }

        RenderResourceFormat GetFormat() const override { return m_Format; }

        bool AllowedRenderTarget() const override { return m_IsRenderTarget; }
        bool AllowedDepthStencil() const override { return m_IsDepthStencil; }
        bool AllowedRandomReadWrites() const override { return m_AllowUAV; }

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }

        DescriptorIndex GetSrvHandle() const { return m_ReadHandle; }
        DescriptorIndex GetUavHandle() const { return m_WriteHandle; }

        void SetRenderTargetHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);
        D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetHandle() const { return m_RenderTargetDescriptor; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilTargetHandle() const { return m_DepthStencilDescriptor; }

    private:
        void CreateViews(GpuResource* resource);

        const char*                     m_Name;
        GpuResource*                    m_Resource;
        uint32_t						m_Width;
        uint32_t						m_Height;
        RenderResourceFormat			m_Format;

        bool							m_IsRenderTarget;
        bool							m_IsDepthStencil;
        bool							m_AllowUAV;

        DescriptorIndex				    m_ReadHandle;
        DescriptorIndex				    m_WriteHandle;
        DescriptorIndex				    m_RenderTargetHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE		m_RenderTargetDescriptor;
        DescriptorIndex				    m_DepthStencilHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE		m_DepthStencilDescriptor;
    };
}