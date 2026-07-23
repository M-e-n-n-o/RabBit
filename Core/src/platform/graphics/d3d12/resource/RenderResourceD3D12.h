#if RB_GRAPHICS_API_D3D12

#pragma once

#include "RabBitCommon.h"
#include "Descriptor.h"
#include "graphics/RenderResource.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    class GpuResource;
    class GpuGuardD3D12;

    class GenericBufferD3D12 : public GenericBuffer
    {
    public:
        GenericBufferD3D12(const char* name, RenderResourceFormat format, uint32_t elements, bool random_read_write_access);
        GenericBufferD3D12(const char* name, uint32_t element_size, uint32_t elements, bool random_read_write_access);
        ~GenericBufferD3D12();

        void* GetNativeResource() const override { return m_Resource; }

        RenderResourceFormat GetFormat() const override { return m_Format; }
        uint64_t GetSize() const override { return m_Elements * m_ElementSize; }

        bool AllowedRandomReadWrites() const override { return m_RandomReadWrite; }

        DescriptorIndex GetSrvHandle();
        DescriptorIndex GetUavHandle();

    private:
        GpuResource*            m_Resource;
        RenderResourceFormat    m_Format;
        uint32_t                m_ElementSize;
        uint32_t                m_Elements;
        bool                    m_RandomReadWrite;

        DescriptorIndex         m_SRV;
        DescriptorIndex         m_UAV;
    };

    class VertexBufferD3D12 : public VertexBuffer
    {
    public:
        VertexBufferD3D12(const char* name, const TopologyType& type, const void* data, uint32_t vertex_size, uint64_t data_size, bool transient);
        ~VertexBufferD3D12();

        void* GetNativeResource() const override { return m_Resource; }

        uint32_t GetVertexSize() const override { return m_VertexSize; }
        uint32_t GetVertexElementCount() const override { return m_Size / m_VertexSize; }

        TopologyType GetTopologyType() const override { return m_Type; }

        const D3D12_VERTEX_BUFFER_VIEW& GetView();

    private:
        GpuResource*                m_Resource;
        D3D12_VERTEX_BUFFER_VIEW    m_View;
        TopologyType                m_Type;
        uint32_t                    m_VertexSize;
        uint64_t                    m_Size;
        const void*                 m_Data;
        bool                        m_Transient;
        D3D12_GPU_VIRTUAL_ADDRESS   m_GpuAddress;
    };

    class IndexBufferD3D12 : public IndexBuffer
    {
    public:
        IndexBufferD3D12(const char* name, const uint32_t* data, uint64_t elements);
        ~IndexBufferD3D12();

        void* GetNativeResource() const override { return m_Resource; }

        uint64_t GetIndexCount() const override { return m_Elements; }

        const D3D12_INDEX_BUFFER_VIEW& GetView();

    private:
        GpuResource*                m_Resource;
        D3D12_INDEX_BUFFER_VIEW		m_View;
        uint64_t					m_Elements;
        const void*                 m_Data;
    };


    class ReadbackBufferD3D12 : public ReadbackBuffer
    {
    public:
        ReadbackBufferD3D12(const char* name, uint64_t size);
        ReadbackBufferD3D12(const char* name, RenderResource* target_size);
        ~ReadbackBufferD3D12();

        void* GetNativeResource() const override { return m_Resource; }

        uint64_t GetSize() const override { return m_Size; }
        uint64_t GetPackedSize() const override { return m_PackedSize; }

        void OnScheduledReadback(Shared<GpuGuardD3D12>& fence);

        bool GetData(void* memory, bool should_block) override;

    private:
        GpuResource*                                m_Resource;
        uint8_t*                                    m_MappedMemory;
        uint64_t                                    m_Size;
        uint64_t                                    m_PackedSize;
        Queue<Shared<GpuGuardD3D12>>                m_Fences;

        List<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>    m_Layouts;
        List<UINT>                                  m_NumRows;
        List<UINT64>                                m_RowSizes;
        List<UINT64>                                m_PackedOffsets;
    };

    class Texture2DD3D12 : public Texture2D
    {
    public:
        Texture2DD3D12(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access);
        Texture2DD3D12(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t mips, bool is_render_target, bool random_read_write_access);
        Texture2DD3D12(const char* name, const void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access);
        Texture2DD3D12(const char* name, const void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t mips, bool is_render_target, bool random_read_write_access);
        Texture2DD3D12(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access);
        Texture2DD3D12(const Texture2DD3D12* other);
        ~Texture2DD3D12();

        void* GetNativeResource() const override { return m_Resource; }

        RenderResourceFormat GetFormat() const override { return m_Format; }

        bool AllowedRenderTarget() const override { return m_IsRenderTarget; }
        bool AllowedDepthStencil() const override { return m_IsDepthStencil; }
        bool AllowedRandomReadWrites() const override { return m_AllowUAV; }

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }

        uint32_t GetViewportWidth() const override { return m_VpWidth; }
        uint32_t GetViewportHeight() const override { return m_VpHeight; }

        void SetViewportWidth(uint32_t width) override;
        void SetViewportHeight(uint32_t height) override;

        uint32_t GetMipCount() const override;
        uint32_t GetBaseMip() const override;

        void SetBaseMip(uint32_t mip) override;
        void SetMipCount(uint32_t mips) override;

        void ResetView() override;

        DescriptorIndex GetSrvHandle() const;
        DescriptorIndex GetUavHandle() const;

        void SetRenderTargetHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);
        D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetHandle() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilTargetHandle() const;

    private:
        void CreateViews(GpuResource* resource);

        GpuResource*                    m_Resource;
        bool                            m_IsAlias;
        uint32_t                        m_Width;
        uint32_t                        m_Height;
        uint32_t                        m_VpWidth;
        uint32_t                        m_VpHeight;
        RenderResourceFormat            m_Format;

        uint32_t                        m_MipCount;
        uint32_t                        m_SetMipCount;
        uint32_t                        m_BaseMip;

        bool                            m_IsRenderTarget;
        bool                            m_IsDepthStencil;
        bool                            m_AllowUAV;
        bool                            m_Typeless;

        DescriptorIndex                 m_ReadHandle;
        DescriptorIndex                 m_WriteHandle;
        DescriptorIndex                 m_RenderTargetHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE     m_RenderTargetDescriptor;
        DescriptorIndex                 m_DepthStencilHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE     m_DepthStencilDescriptor;
    };

    class Texture2DArrayD3D12 : public Texture2DArray
    {
    public:
        Texture2DArrayD3D12(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t slices, bool is_render_target, bool random_read_write_access);
        Texture2DArrayD3D12(const Texture2DArrayD3D12* other);
        ~Texture2DArrayD3D12();

        void* GetNativeResource() const override { return m_Resource; }

        RenderResourceFormat GetFormat() const override { return m_Format; }

        bool AllowedRenderTarget() const override { return m_IsRenderTarget; }
        bool AllowedDepthStencil() const override { return m_IsDepthStencil; }
        bool AllowedRandomReadWrites() const override { return m_AllowUAV; }

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }

        uint32_t GetViewportWidth() const override { return m_VpWidth; }
        uint32_t GetViewportHeight() const override { return m_VpHeight; }

        void SetViewportWidth(uint32_t width) override;
        void SetViewportHeight(uint32_t height) override;

        uint32_t GetMipCount() const override;
        uint32_t GetBaseMip() const override;

        void SetBaseMip(uint32_t mip) override;
        void SetMipCount(uint32_t mips) override;

        uint32_t GetArraySize() const override;
        uint32_t GetFirstArraySlice() const override;

        void SetArraySize(uint32_t size) override;
        void SetFirstArraySlice(uint32_t slice) override;

        void ResetView() override;

        DescriptorIndex GetSrvHandle() const;
        DescriptorIndex GetUavHandle() const;

        D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetHandle() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilTargetHandle() const;

    private:
        void CreateViews(GpuResource* resource);

        bool                            m_IsAlias;
        GpuResource*                    m_Resource;
        uint32_t                        m_Width;
        uint32_t                        m_Height;
        uint32_t                        m_VpWidth;
        uint32_t                        m_VpHeight;
        uint32_t                        m_Slices;
        uint32_t                        m_SetSlices;
        uint32_t                        m_BaseSlice;
        RenderResourceFormat            m_Format;

        bool                            m_IsRenderTarget;
        bool                            m_IsDepthStencil;
        bool                            m_AllowUAV;
        bool                            m_Typeless;

        DescriptorIndex                 m_ReadHandle;
        DescriptorIndex                 m_WriteHandle;
        DescriptorIndex                 m_RenderTargetHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE     m_RenderTargetDescriptor;
        DescriptorIndex                 m_DepthStencilHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE     m_DepthStencilDescriptor;
    };

    class Texture3DD3D12 : public Texture3D
    {
    public:
        Texture3DD3D12(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t depth, bool is_render_target, bool random_read_write_access);
        ~Texture3DD3D12();

        void* GetNativeResource() const override { return m_Resource; }

        RenderResourceFormat GetFormat() const override { return m_Format; }

        bool AllowedRandomReadWrites() const override { return m_AllowUAV; }

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }
        uint32_t GetDepth() const override { return m_Depth; }
        uint32_t GetViewportWidth() const override { return m_VpWidth; }
        uint32_t GetViewportHeight() const override { return m_VpHeight; }
        uint32_t GetViewportDepth() const override { return m_VpDepth; }

        void SetViewportWidth(uint32_t width) override;
        void SetViewportHeight(uint32_t height) override;
        void SetViewportDepth(uint32_t depth) override;

        bool AllowedRenderTarget() const override { return m_IsRenderTarget; }

        uint32_t GetMipCount() const override;
        uint32_t GetBaseMip() const override;

        void SetBaseMip(uint32_t mip) override;
        void SetMipCount(uint32_t mips) override;

        void ResetView() override;

    private:
        void CreateViews(GpuResource* resource);

        GpuResource*                    m_Resource;
        RenderResourceFormat            m_Format;
                                        
        uint32_t                        m_Width;
        uint32_t                        m_Height;
        uint32_t                        m_Depth;
        uint32_t                        m_VpWidth;
        uint32_t                        m_VpHeight;
        uint32_t                        m_VpDepth;
                                        
        bool                            m_IsRenderTarget;
        bool                            m_AllowUAV;

        DescriptorIndex                 m_ReadHandle;
        DescriptorIndex                 m_UavHandle;
        DescriptorIndex                 m_RenderTargetHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE     m_RenderTargetDescriptor;
    };
}
#endif