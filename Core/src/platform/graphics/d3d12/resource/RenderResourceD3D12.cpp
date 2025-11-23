#if RB_GRAPHICS_API_D3D12

#include "RabBitCommon.h"
#include "RenderResourceD3D12.h"
#include "ResourceManager.h"
#include "app/Application.h"
#include "graphics/Renderer.h"
#include "graphics/ResourceStreamer.h"
#include "platform/graphics/d3d12/UtilsD3D12.h"
#include "platform/graphics/d3d12/GraphicsDevice.h"
#include "platform/graphics/d3d12/resource/UploadAllocator.h"

namespace RB::Graphics::D3D12
{
    // ---------------------------------------------------------------------------
    //								VertexBuffer
    // ---------------------------------------------------------------------------

    VertexBufferD3D12::VertexBufferD3D12(const char* name, const TopologyType& type, void* data, uint32_t vertex_size, uint64_t data_size, bool transient)
        : m_Name(name)
        , m_Type(type)
        , m_VertexSize(vertex_size)
        , m_Size(data_size)
        , m_Data(data)
        , m_View{}
        , m_Transient(transient)
        , m_GpuAddress(0)
    {
        if (m_Transient)
        {
            UploadAllocation alloc = g_TransientVBAllocator->Allocate(data_size);
            m_Resource   = alloc.resource;
            m_GpuAddress = alloc.gpuAddress;

            memcpy(alloc.cpuWriteAddress, data, data_size);
        }
        else
        {
            m_Resource = new GpuResource();
            g_ResourceManager->ScheduleCreateVertexResource(m_Resource, name, { data_size });

            Streamable streamable = {};
            streamable.resource     = this;
            streamable.uploadData   = data;
            streamable.uploadSize   = data_size;
            Application::GetInstance()->GetRenderer()->GetStreamer()->ScheduleUpload(streamable);
        }
    }

    VertexBufferD3D12::~VertexBufferD3D12()
    {
        if (!m_Transient)
        {
            SAFE_DELETE(m_Resource);
        }
    }

    D3D12_VERTEX_BUFFER_VIEW VertexBufferD3D12::GetView()
    {
        if (m_View.SizeInBytes == 0)
        {
            m_View.BufferLocation = m_GpuAddress > 0 ? m_GpuAddress : m_Resource->GetResource()->GetGPUVirtualAddress();
            m_View.SizeInBytes    = m_Size;
            m_View.StrideInBytes  = m_VertexSize;
        }

        return m_View;
    }

    // ---------------------------------------------------------------------------
    //								IndexBuffer
    // ---------------------------------------------------------------------------

    IndexBufferD3D12::IndexBufferD3D12(const char* name, uint32_t* data, uint64_t elements)
        : m_Name(name)
        , m_Elements(elements)
        , m_Data(data)
        , m_View{}
    {
        uint64_t size = m_Elements * GetElementSizeFromFormat(GetFormat());

        m_Resource = new GpuResource();
        g_ResourceManager->ScheduleCreateIndexResource(m_Resource, name, { size });

        Streamable streamable = {};
        streamable.resource     = this;
        streamable.uploadData   = data;
        streamable.uploadSize   = size;
        Application::GetInstance()->GetRenderer()->GetStreamer()->ScheduleUpload(streamable);
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
            m_View.SizeInBytes    = m_Elements * sizeof(uint32_t);
            m_View.Format         = ConvertToDXGIFormat(GetFormat());
        }

        return m_View;
    }

    // ---------------------------------------------------------------------------
    //								TextureUtil
    // ---------------------------------------------------------------------------

    DescriptorIndex CreateSRV(ID3D12Resource* res, bool depth_stencil, bool typeless, uint32_t mip_levels, uint32_t base_mip, RenderResourceFormat format, bool transient)
    {
        if (depth_stencil && !typeless)
        {
            return DescriptorIndex{};
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format                         = ConvertToDXGIFormat(format);
        desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels            = mip_levels;
        desc.Texture2D.MostDetailedMip      = base_mip;
        desc.Texture2D.PlaneSlice           = 0;
        desc.Texture2D.ResourceMinLODClamp  = 0.0f;

        return g_DescriptorManager->CreateDescriptor(res, desc, transient);
    }

    DescriptorIndex CreateSRV(ID3D12Resource* res, bool depth_stencil, bool typeless, uint32_t mip_levels, uint32_t base_mip, uint32_t slices, uint32_t base_slice, RenderResourceFormat format, bool transient)
    {
        if (depth_stencil && !typeless)
        {
            return DescriptorIndex{};
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format                              = ConvertToDXGIFormat(format);
        desc.ViewDimension                       = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        desc.Shader4ComponentMapping             = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2DArray.MipLevels            = mip_levels;
        desc.Texture2DArray.MostDetailedMip      = base_mip;
        desc.Texture2DArray.FirstArraySlice      = base_slice;
        desc.Texture2DArray.ArraySize            = slices;
        desc.Texture2DArray.PlaneSlice           = 0;
        desc.Texture2DArray.ResourceMinLODClamp  = 0.0f;

        return g_DescriptorManager->CreateDescriptor(res, desc, transient);
    }

    DescriptorIndex CreateUAV(ID3D12Resource* res, bool allow_uav, uint32_t mip_slice, RenderResourceFormat format, bool transient)
    {
        if (!allow_uav)
        {
            return DescriptorIndex{};
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.Format                 = ConvertToDXGIFormat(format);
        desc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice     = mip_slice;
        desc.Texture2D.PlaneSlice   = 0;

        return g_DescriptorManager->CreateDescriptor(res, desc, transient);
    }

    DescriptorIndex CreateUAV(ID3D12Resource* res, bool allow_uav, uint32_t mip_slice, uint32_t slices, uint32_t base_slice, RenderResourceFormat format, bool transient)
    {
        if (!allow_uav)
        {
            return DescriptorIndex{};
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.Format                         = ConvertToDXGIFormat(format);
        desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MipSlice        = mip_slice;
        desc.Texture2DArray.FirstArraySlice = base_slice;
        desc.Texture2DArray.ArraySize       = slices;
        desc.Texture2DArray.PlaneSlice      = 0;

        return g_DescriptorManager->CreateDescriptor(res, desc, transient);
    }

    DescriptorIndex CreateRTV(ID3D12Resource* res, bool is_rendertarget, uint32_t mip_slice, RenderResourceFormat format, bool transient)
    {
        if (!is_rendertarget)
        {
            return DescriptorIndex{};
        }

        D3D12_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format                 = ConvertToDXGIFormat(format);
        desc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice     = mip_slice;
        desc.Texture2D.PlaneSlice   = 0;

        return g_DescriptorManager->CreateDescriptor(res, desc, transient);
    }

    DescriptorIndex CreateRTV(ID3D12Resource* res, bool is_rendertarget, uint32_t mip_slice, uint32_t slices, uint32_t base_slice, RenderResourceFormat format, bool transient)
    {
        if (!is_rendertarget)
        {
            return DescriptorIndex{};
        }

        D3D12_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format                         = ConvertToDXGIFormat(format);
        desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MipSlice        = mip_slice;
        desc.Texture2DArray.FirstArraySlice = base_slice;
        desc.Texture2DArray.ArraySize       = slices;
        desc.Texture2DArray.PlaneSlice      = 0;

        return g_DescriptorManager->CreateDescriptor(res, desc, transient);
    }

    DescriptorIndex CreateDSV(ID3D12Resource* res, bool depth_stencil, uint32_t mip_slice, RenderResourceFormat format, bool transient)
    {
        if (!depth_stencil)
        {
            return DescriptorIndex{};
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format             = ConvertToDXGIFormat(format, true, true);
        desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        desc.Flags              = D3D12_DSV_FLAG_NONE;
        desc.Texture2D.MipSlice = mip_slice;

        return g_DescriptorManager->CreateDescriptor(res, desc, transient);
    }

    DescriptorIndex CreateDSV(ID3D12Resource* res, bool depth_stencil, uint32_t mip_slice, uint32_t slices, uint32_t base_slice, RenderResourceFormat format, bool transient)
    {
        if (!depth_stencil)
        {
            return DescriptorIndex{};
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format                         = ConvertToDXGIFormat(format, true, true);
        desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        desc.Flags                          = D3D12_DSV_FLAG_NONE;
        desc.Texture2DArray.MipSlice        = mip_slice;
        desc.Texture2DArray.FirstArraySlice = base_slice;
        desc.Texture2DArray.ArraySize       = slices;

        return g_DescriptorManager->CreateDescriptor(res, desc, transient);
    }

    // ---------------------------------------------------------------------------
    //								Texture2D
    // ---------------------------------------------------------------------------

    Texture2DD3D12::Texture2DD3D12(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access)
        : m_Name(name)
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
        m_Typeless = IsTypelessFormat(format);

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        if (m_IsRenderTarget)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (m_AllowUAV)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if (m_IsDepthStencil)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            if (!m_Typeless)
                flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }

        // TODO Add mip support

        ResourceManager::Texture2DDesc desc = {};
        desc.format     = ConvertToDXGIFormat(m_Format, false);
        desc.width      = m_Width;
        desc.height     = m_Height;
        desc.arraySize  = 1;
        desc.mipLevels  = 1;
        desc.flags      = flags;

        g_ResourceManager->ScheduleCreateTexture2DResource(m_Resource, name, desc);
    }

    Texture2DD3D12::Texture2DD3D12(const char* name, void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access)
        : Texture2DD3D12(name, format, width, height, is_render_target, random_read_write_access)
    {
        Streamable streamable = {};
        streamable.resource     = this;
        streamable.uploadData   = data;
        streamable.uploadSize   = data_size;
        Application::GetInstance()->GetRenderer()->GetStreamer()->ScheduleUpload(streamable);
    }

    Texture2DD3D12::Texture2DD3D12(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access)
        : m_Name(name)
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


    // TODO: Setting any of the following methods will cause the GetXHandle methods
    // to return a transient view using the newly set amount of mips.

    uint32_t Texture2DD3D12::GetMipCount() const
    {
        // TODO Add mip support
        return 1;
    }

    uint32_t Texture2DD3D12::GetBaseMip() const
    {
        // TODO Add mip support
        return 0;
    }

    void Texture2DD3D12::SetBaseMip(uint32_t mip)
    {
        // TODO Add mip support
    }

    void Texture2DD3D12::SetMipCount(uint32_t mips)
    {
        // TODO Add mip support
    }

    void Texture2DD3D12::ResetView()
    {
        // TODO Add mip support
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
        // TODO Add mip support

        m_ReadHandle         = CreateSRV(m_Resource->GetResource(), m_IsDepthStencil, m_Typeless, 1, 0, m_Format, false);
        m_WriteHandle        = CreateUAV(m_Resource->GetResource(), m_AllowUAV, 0, m_Format, false);
        m_RenderTargetHandle = CreateRTV(m_Resource->GetResource(), m_IsRenderTarget, 0, m_Format, false);
        m_DepthStencilHandle = CreateDSV(m_Resource->GetResource(), m_IsDepthStencil, 0, m_Format, false);

        if (m_RenderTargetHandle.isValid())
            m_RenderTargetDescriptor = g_DescriptorManager->GetCpuHandle(m_RenderTargetHandle);
        if (m_DepthStencilHandle.isValid())
            m_DepthStencilDescriptor = g_DescriptorManager->GetCpuHandle(m_DepthStencilHandle);
    }

    // ---------------------------------------------------------------------------
    //								Texture2DArray
    // ---------------------------------------------------------------------------

    Texture2DArrayD3D12::Texture2DArrayD3D12(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t slices, bool is_render_target, bool random_read_write_access)
        : m_Name(name)
        , m_Format(format)
        , m_Width(width)
        , m_Height(height)
        , m_Slices(slices)
        , m_SetSlices(slices)
        , m_BaseSlice(0)
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

        m_Resource = new GpuResource(std::bind(&Texture2DArrayD3D12::CreateViews, this, std::placeholders::_1));

        m_IsDepthStencil = IsDepthFormat(format);
        m_Typeless = IsTypelessFormat(format);

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        if (m_IsRenderTarget)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (m_AllowUAV)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        if (m_IsDepthStencil)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            if (!m_Typeless)
                flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }

        // TODO Add mip support

        ResourceManager::Texture2DDesc desc = {};
        desc.format     = ConvertToDXGIFormat(m_Format, false);
        desc.width      = m_Width;
        desc.height     = m_Height;
        desc.arraySize  = slices;
        desc.mipLevels  = 1;
        desc.flags      = flags;

        g_ResourceManager->ScheduleCreateTexture2DResource(m_Resource, name, desc);
    }

    Texture2DArrayD3D12::~Texture2DArrayD3D12()
    {
        g_DescriptorManager->InvalidateDescriptor(m_ReadHandle);
        g_DescriptorManager->InvalidateDescriptor(m_WriteHandle);
        g_DescriptorManager->InvalidateDescriptor(m_RenderTargetHandle);
        g_DescriptorManager->InvalidateDescriptor(m_DepthStencilHandle);

        SAFE_DELETE(m_Resource);
    }

    uint32_t Texture2DArrayD3D12::GetMipCount() const
    {
        // TODO Add mip support
        return 1;
    }

    uint32_t Texture2DArrayD3D12::GetBaseMip() const
    {
        // TODO Add mip support
        return 0;
    }

    void Texture2DArrayD3D12::SetBaseMip(uint32_t mip)
    {
        // TODO Add mip support
    }

    void Texture2DArrayD3D12::SetMipCount(uint32_t mips)
    {
        // TODO Add mip support
    }

    uint32_t Texture2DArrayD3D12::GetArraySize() const
    {
        return m_SetSlices;
    }

    uint32_t Texture2DArrayD3D12::GetFirstArraySlice() const
    {
        return m_BaseSlice;
    }

    void Texture2DArrayD3D12::SetArraySize(uint32_t size)
    {
        m_SetSlices = size;
    }

    void Texture2DArrayD3D12::SetFirstArraySlice(uint32_t slice)
    {
        m_BaseSlice = slice;
        m_SetSlices = m_Slices - slice;
    }

    void Texture2DArrayD3D12::ResetView()
    {
        m_BaseSlice = 0;
        m_SetSlices = m_Slices;
    }

    DescriptorIndex Texture2DArrayD3D12::GetSrvHandle() const
    {
        if (m_ReadHandle.isValid() && (m_BaseSlice > 0 || m_SetSlices != m_Slices))
        {
            // Create a transient descriptor with the set properties
            return CreateSRV(m_Resource->GetResource(), m_IsDepthStencil, m_Typeless, 1, 0, m_SetSlices, m_BaseSlice, m_Format, true);
        }

        return m_ReadHandle;
    }

    DescriptorIndex Texture2DArrayD3D12::GetUavHandle() const
    {
        if (m_WriteHandle.isValid() && (m_BaseSlice > 0 || m_SetSlices != m_Slices))
        {
            // Create a transient descriptor with the set properties
            return CreateUAV(m_Resource->GetResource(), m_AllowUAV, 0, m_SetSlices, m_BaseSlice, m_Format, true);
        }

        return m_WriteHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Texture2DArrayD3D12::GetRenderTargetHandle() const
    {
        if (m_RenderTargetHandle.isValid() && (m_BaseSlice > 0 || m_SetSlices != m_Slices))
        {
            // Create a transient descriptor with the set properties
            DescriptorIndex temp = CreateRTV(m_Resource->GetResource(), m_IsRenderTarget, 0, m_SetSlices, m_BaseSlice, m_Format, true);
            return g_DescriptorManager->GetCpuHandle(temp);
        }

        return m_RenderTargetDescriptor;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Texture2DArrayD3D12::GetDepthStencilTargetHandle() const
    {
        if (m_DepthStencilHandle.isValid() && (m_BaseSlice > 0 || m_SetSlices != m_Slices))
        {
            // Create a transient descriptor with the set properties
            DescriptorIndex temp = CreateDSV(m_Resource->GetResource(), m_IsDepthStencil, 0, m_SetSlices, m_BaseSlice, m_Format, true);
            return g_DescriptorManager->GetCpuHandle(temp);
        }

        return m_DepthStencilDescriptor;
    }

    void Texture2DArrayD3D12::CreateViews(GpuResource* /*resource*/)
    {
        // TODO Add mip support

        m_ReadHandle         = CreateSRV(m_Resource->GetResource(), m_IsDepthStencil, m_Typeless, 1, 0, m_Slices, 0, m_Format, false);
        m_WriteHandle        = CreateUAV(m_Resource->GetResource(), m_AllowUAV, 0, m_Slices, 0, m_Format, false);
        m_RenderTargetHandle = CreateRTV(m_Resource->GetResource(), m_IsRenderTarget, 0, m_Slices, 0, m_Format, false);
        m_DepthStencilHandle = CreateDSV(m_Resource->GetResource(), m_IsDepthStencil, 0, m_Slices, 0, m_Format, false);

        if (m_RenderTargetHandle.isValid())
            m_RenderTargetDescriptor = g_DescriptorManager->GetCpuHandle(m_RenderTargetHandle);
        if (m_DepthStencilHandle.isValid())
            m_DepthStencilDescriptor = g_DescriptorManager->GetCpuHandle(m_DepthStencilHandle);
    }
}
#endif