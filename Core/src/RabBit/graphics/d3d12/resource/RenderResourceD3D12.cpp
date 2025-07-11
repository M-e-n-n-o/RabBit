#include "RabBitCommon.h"
#include "RenderResourceD3D12.h"
#include "ResourceManager.h"
#include "graphics/d3d12/UtilsD3D12.h"
#include "graphics/d3d12/GraphicsDevice.h"
#include "app/Application.h"
#include "graphics/Renderer.h"
#include "graphics/ResourceStreamer.h"

namespace RB::Graphics::D3D12
{
    // ---------------------------------------------------------------------------
    //								VertexBuffer
    // ---------------------------------------------------------------------------

    VertexBufferD3D12::VertexBufferD3D12(const char* name, const TopologyType& type, void* data, uint32_t vertex_size, uint64_t data_size)
        : m_Name(name)
        , m_Type(type)
        , m_VertexSize(vertex_size)
        , m_Size(data_size)
        , m_Data(data)
        , m_View{}
    {
        m_Resource = new GpuResource();
        g_ResourceManager->ScheduleCreateVertexResource(m_Resource, name, { data_size });

        Streamable streamable = {};
        streamable.resource     = this;
        streamable.uploadData   = data;
        streamable.uploadSize   = data_size;
        Application::GetInstance()->GetRenderer()->GetStreamer()->ScheduleForStream(streamable);
    }

    VertexBufferD3D12::~VertexBufferD3D12()
    {
        SAFE_DELETE(m_Resource);
    }

    D3D12_VERTEX_BUFFER_VIEW VertexBufferD3D12::GetView()
    {
        if (m_View.SizeInBytes == 0)
        {
            m_View.BufferLocation = m_Resource->GetResource()->GetGPUVirtualAddress();
            m_View.SizeInBytes    = m_Size;
            m_View.StrideInBytes  = m_VertexSize;
        }

        return m_View;
    }

    // ---------------------------------------------------------------------------
    //								IndexBuffer
    // ---------------------------------------------------------------------------

    IndexBufferD3D12::IndexBufferD3D12(const char* name, uint32_t* data, uint64_t data_size)
        : m_Name(name)
        , m_Size(data_size)
        , m_Data(data)
        , m_View{}
    {
        m_Resource = new GpuResource();
        g_ResourceManager->ScheduleCreateIndexResource(m_Resource, name, { data_size });

        Streamable streamable = {};
        streamable.resource     = this;
        streamable.uploadData   = data;
        streamable.uploadSize   = data_size;
        Application::GetInstance()->GetRenderer()->GetStreamer()->ScheduleForStream(streamable);
    }

    IndexBufferD3D12::~IndexBufferD3D12()
    {
        SAFE_DELETE(m_Resource);
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferD3D12::GetView()
    {
        if (m_View.SizeInBytes == 0)
        {
            m_View.BufferLocation = m_Resource->GetResource()->GetGPUVirtualAddress();
            m_View.SizeInBytes    = m_Size;
            m_View.Format         = ConvertToDXGIFormat(GetFormat());
        }

        return m_View;
    }

    // ---------------------------------------------------------------------------
    //								Texture2D
    // ---------------------------------------------------------------------------

    Texture2DD3D12::Texture2DD3D12(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space)
        : Texture2D(color_space)
        , m_Name(name)
        , m_Format(format)
        , m_Width(width)
        , m_Height(height)
        , m_IsRenderTarget(is_render_target)
        , m_AllowUAV(random_read_write_access)
        , m_ReadHandle({})
        , m_WriteHandle({})
        , m_RenderTargetHandle({})
        , m_DepthStencilHandle({})
        , m_RenderTargetDescriptor({})
        , m_DepthStencilDescriptor({})
    {
        RB_ASSERT_FATAL(LOGTAG_GRAPHICS, width > 0 && height > 0, "Cannot create a texture with a width or height smaller than 1");

        m_Resource = new GpuResource(std::bind(&Texture2DD3D12::CreateViews, this, std::placeholders::_1));

        m_IsDepthStencil = IsDepthFormat(format);

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        if (m_IsRenderTarget)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (m_AllowUAV)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if (m_IsDepthStencil)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }

        // TODO Add mip support

        ResourceManager::Texture2DDesc desc = {};
        desc.format     = ConvertToDXGIFormat(m_Format);
        desc.width      = m_Width;
        desc.height     = m_Height;
        desc.arraySize  = 1;
        desc.mipLevels  = 1;
        desc.flags      = flags;

        g_ResourceManager->ScheduleCreateTexture2DResource(m_Resource, name, desc);
    }

    Texture2DD3D12::Texture2DD3D12(const char* name, void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space)
        : Texture2DD3D12(name, format, width, height, is_render_target, random_read_write_access, color_space)
    {
        Streamable streamable = {};
        streamable.resource     = this;
        streamable.uploadData   = data;
        streamable.uploadSize   = data_size;
        Application::GetInstance()->GetRenderer()->GetStreamer()->ScheduleForStream(streamable);
    }

    Texture2DD3D12::Texture2DD3D12(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space)
        : Texture2D(color_space)
        , m_Name(name)
        , m_Resource((GpuResource*)internal_resource)
        , m_Format(format)
        , m_Width(width)
        , m_Height(height)
        , m_IsRenderTarget(is_render_target)
        , m_AllowUAV(random_read_write_access)
        , m_ReadHandle({})
        , m_WriteHandle({})
        , m_RenderTargetHandle({})
        , m_DepthStencilHandle({})
        , m_RenderTargetDescriptor({})
        , m_DepthStencilDescriptor({})
    {
        m_IsDepthStencil = IsDepthFormat(format);
    }

    Texture2DD3D12::~Texture2DD3D12()
    {
        g_DescriptorManager->InvalidateDescriptor(m_ReadHandle);
        g_DescriptorManager->InvalidateDescriptor(m_WriteHandle);
        g_DescriptorManager->InvalidateDescriptor(m_RenderTargetHandle);
        g_DescriptorManager->InvalidateDescriptor(m_DepthStencilHandle);

        SAFE_DELETE(m_Resource);
    }

    void Texture2DD3D12::SetRenderTargetHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        if (m_RenderTargetHandle.isValid())
        {
            g_DescriptorManager->InvalidateDescriptor(m_RenderTargetHandle);
        }

        m_RenderTargetDescriptor = handle;
    }

    void Texture2DD3D12::CreateViews(GpuResource* /*resource*/)
    {
        // SRV
        if (!m_IsDepthStencil)
        {
            // TODO Add mip support

            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format                         = ConvertToDXGIFormat(m_Format);
            desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
            desc.Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Texture2D.MipLevels            = 1;
            desc.Texture2D.MostDetailedMip      = 0;
            desc.Texture2D.PlaneSlice           = 0;
            desc.Texture2D.ResourceMinLODClamp  = 0.0f;

            m_ReadHandle = g_DescriptorManager->CreateDescriptor(m_Resource->GetResource().Get(), desc);
        }

        // UAV
        if (m_AllowUAV)
        {
            // TODO Add mip support

            D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format                 = ConvertToDXGIFormat(m_Format);
            desc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice     = 0;
            desc.Texture2D.PlaneSlice   = 0;

            m_WriteHandle = g_DescriptorManager->CreateDescriptor(m_Resource->GetResource().Get(), desc);
        }

        if (m_IsRenderTarget)
        {
            // TODO Add mip support

            D3D12_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format                 = ConvertToDXGIFormat(m_Format);
            desc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice     = 0;
            desc.Texture2D.PlaneSlice   = 0;

            m_RenderTargetHandle     = g_DescriptorManager->CreateDescriptor(m_Resource->GetResource().Get(), desc);
            m_RenderTargetDescriptor = g_DescriptorManager->GetCpuHandle(m_RenderTargetHandle);
        }

        if (m_IsDepthStencil)
        {
            // TODO Add mip support

            D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format             = ConvertToDXGIFormat(m_Format);
            desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
            desc.Flags              = D3D12_DSV_FLAG_NONE;
            desc.Texture2D.MipSlice = 0;

            m_DepthStencilHandle     = g_DescriptorManager->CreateDescriptor(m_Resource->GetResource().Get(), desc);
            m_DepthStencilDescriptor = g_DescriptorManager->GetCpuHandle(m_DepthStencilHandle);
        }
    }
}