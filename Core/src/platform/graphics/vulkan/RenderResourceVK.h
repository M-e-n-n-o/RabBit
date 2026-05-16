#if RB_GRAPHICS_API_VULKAN

#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

#include <vulkan/vulkan.h>

namespace RB::Graphics::VK
{
    class GpuResource;

    class VertexBufferVK : public VertexBuffer
    {
    public:
        VertexBufferVK(const char* name, const TopologyType& type, void* data, uint32_t vertex_size, uint64_t data_size, bool transient);
        ~VertexBufferVK();

        const char* GetName() const override { return m_Name; }

        void* GetNativeResource() const override { return m_Resource; }

        uint32_t GetVertexElementCount() const override { return m_Size / m_VertexSize; }

        TopologyType GetTopologyType() const override { return m_Type; }

    private:
        const char*     m_Name;
        GpuResource*    m_Resource;
        VkBufferView    m_View;
        TopologyType	m_Type;
        uint32_t		m_VertexSize;
        uint64_t		m_Size;
    };

    class IndexBufferVK : public IndexBuffer
    {
    public:
        IndexBufferVK(const char* name, uint32_t* data, uint64_t elements);
        ~IndexBufferVK();

        const char* GetName() const override { return m_Name; }

        void* GetNativeResource() const override { return m_Resource; }

        uint64_t GetIndexCount() const override { return m_Elements; }

    private:
        const char*     m_Name;
        GpuResource*    m_Resource;
        VkBufferView    m_View;
        uint64_t	    m_Elements;
    };

    class Texture2DVK : public Texture2D
    {
    public:
        Texture2DVK(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access);
        Texture2DVK(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access);
        Texture2DVK(const Texture2DVK* other);
        ~Texture2DVK();

        const char* GetName() const override { return m_Name; }

        void* GetNativeResource() const override { return m_Resource; }

        RenderResourceFormat GetFormat() const override { return m_Format; }

        bool AllowedRenderTarget() const override { return m_IsRenderTarget; }
        bool AllowedDepthStencil() const override { return m_IsDepthStencil; }
        bool AllowedRandomReadWrites() const override { return m_AllowReadWrite; }

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

        void SetView(VkImageView image_view);

    private:
        const char*             m_Name;
        GpuResource*            m_Resource;
        VkImageView             m_ImageView;
        uint32_t                m_Width;
        uint32_t                m_Height;
        uint32_t                m_VpWidth;
        uint32_t                m_VpHeight;
        RenderResourceFormat    m_Format;

        bool                    m_IsRenderTarget;
        bool                    m_IsDepthStencil;
        bool                    m_AllowReadWrite;
    };
}
#endif