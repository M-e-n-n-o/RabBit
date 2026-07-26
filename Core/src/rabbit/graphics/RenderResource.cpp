#include "RabBitCommon.h"
#include "RenderResource.h"
#include "Renderer.h"

#if RB_GRAPHICS_API_D3D12
#include "platform/graphics/d3d12/resource/RenderResourceD3D12.h"
#endif

#if RB_GRAPHICS_API_VULKAN
#include "platform/graphics/vulkan/RenderResourceVK.h"
#endif

namespace RB::Graphics
{
    uint32_t GetElementSizeFromFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case(RenderResourceFormat::RGBA32_FLOAT):
            return 16;
        case(RenderResourceFormat::RG32_FLOAT):
        case(RenderResourceFormat::RGBA16_FLOAT):
            return 8;
        case(RenderResourceFormat::R32_UINT):
        case(RenderResourceFormat::RGBA8_SRGB):
        case(RenderResourceFormat::BGRA8_UNORM):
        case(RenderResourceFormat::RGBA8_UNORM):
        case(RenderResourceFormat::RG16_FLOAT):
        case(RenderResourceFormat::RG16_UINT):
        case(RenderResourceFormat::R32_FLOAT):
        case(RenderResourceFormat::D32_FLOAT):
        case(RenderResourceFormat::R32_TYPELESS):
            return 4;
        case(RenderResourceFormat::RG8_UNORM):
        case(RenderResourceFormat::R16_FLOAT):
        case(RenderResourceFormat::R16_UINT):
        case(RenderResourceFormat::R16_UNORM):
        case(RenderResourceFormat::R16_SNORM):
        case(RenderResourceFormat::D16_UNORM):
            return 2;
        case(RenderResourceFormat::R8_UNORM):
        case(RenderResourceFormat::R8_UINT):
            return 1;
        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Format does not have an element size or is not yet supported");
            return 0;
        }
    }

    uint32_t GetBytesPerBlockFromFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case RenderResourceFormat::BC1_UNORM:
        case RenderResourceFormat::BC1_SRGB:
        case RenderResourceFormat::BC4_UNORM:
            return 8;
        case RenderResourceFormat::BC3_UNORM:
        case RenderResourceFormat::BC3_SRGB:
        case RenderResourceFormat::BC5_UNORM:
            return 16;
        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Format does not have a block size or is not yet supported");
            return 0;
        }
    }

    bool IsDepthFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case RenderResourceFormat::D32_FLOAT:
        case RenderResourceFormat::D16_UNORM:
        case RenderResourceFormat::R32_TYPELESS:
            return true;
        default:
            return false;
        }
    }

    bool IsSRGBFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case RenderResourceFormat::RGBA8_SRGB:
            return true;
        default:
            return false;
        }
    }

    bool IsTypelessFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case RenderResourceFormat::R32_TYPELESS:
            return true;
        default:
            return false;
        }
    }

    bool IsBlockCompressedFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case RenderResourceFormat::BC1_UNORM:
        case RenderResourceFormat::BC1_SRGB:
        case RenderResourceFormat::BC3_UNORM:
        case RenderResourceFormat::BC3_SRGB:
        case RenderResourceFormat::BC4_UNORM:
        case RenderResourceFormat::BC5_UNORM:
            return true;
        default:
            return false;
        }
    }

    uint32_t CalculateMaxMips(uint32_t width, uint32_t height)
    {
        uint32_t max_dim = Math::Max(width, height);

        uint32_t mips = 1;
        while (max_dim > 1)
        {
            max_dim >>= 1;
            ++mips;
        }

        return mips;
    }

    float Texture::GetAspectRatio() const
    {
        return (float)GetWidth() / (float)GetHeight();
    }

    float Texture::GetViewportAspectRatio() const
    {
        return (float)GetViewportWidth() / (float)GetViewportHeight();
    }

    RenderResource::~RenderResource()
    {
        SAFE_DELETE(m_IsStreaming);
    }

    bool RenderResource::ReadyToRender(bool block_until_ready)
    {
        // TODO: This method is not safe for multiple threads to enter at the same time!
        // (which currently does not happen because there is only 1 render thread, but maybe in the future a problem)

        if (m_IsStreaming == nullptr)
            return true;

        if (block_until_ready)
        {
            m_IsStreaming->WaitUntilConditionMet([](const bool& streaming) -> bool
                {
                    return streaming == false;
                });
            SAFE_DELETE(m_IsStreaming);
            return true;
        }
        else if (m_IsStreaming->GetValue() == false)
        {
            SAFE_DELETE(m_IsStreaming);
            return true;
        }

        return false;
    }

    void RenderResource::SetStreaming(bool is_streaming)
    {
        if (is_streaming)
        {
            RB_ASSERT(LOGTAG_GRAPHICS, m_IsStreaming == nullptr, "This resource was already set as streaming before. Its not valid to do it again!");
            m_IsStreaming = new ThreadedVariable<bool>(true);
        }
        else
        {
            RB_ASSERT(LOGTAG_GRAPHICS, m_IsStreaming != nullptr, "This resource was not yet set as streaming");
            m_IsStreaming->SetValue(false);
        }
    }

    RenderResourceType RenderResource::GetPrimitiveType() const
    {
        uint32_t last_primitive = (uint32_t)RenderResourceType::kLastPrimitiveType;
        uint32_t and_value = last_primitive + (last_primitive - 1);

        uint32_t primitive_type = (uint32_t)m_Type & and_value;

        return (RenderResourceType)primitive_type;
    }

    Shared<GenericBuffer> GenericBuffer::Create(const char* name, RenderResourceFormat format, uint32_t elements, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::GenericBufferD3D12>(name, format, elements, random_read_write_access);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<GenericBuffer> GenericBuffer::Create(const char* name, uint32_t element_size, uint32_t elements, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::GenericBufferD3D12>(name, element_size, elements, random_read_write_access);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<VertexBuffer> VertexBuffer::Create(const char* name, const TopologyType& type, const void* data, uint32_t vertex_size, uint64_t data_size, bool transient)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::VertexBufferD3D12>(name, type, data, vertex_size, data_size, transient);
#endif

#if RB_GRAPHICS_API_VULKAN
        case RenderAPI::Vulkan:
            return CreateShared<VK::VertexBufferVK>(name, type, data, vertex_size, data_size, transient);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<IndexBuffer> IndexBuffer::Create(const char* name, const uint32_t* data, uint64_t elements)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::IndexBufferD3D12>(name, data, elements);
#endif

#if RB_GRAPHICS_API_VULKAN
        case RenderAPI::Vulkan:
            return CreateShared<VK::IndexBufferVK>(name, data, elements);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<ReadbackBuffer> ReadbackBuffer::Create(const char* name, uint64_t size)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::ReadbackBufferD3D12>(name, size);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<ReadbackBuffer> ReadbackBuffer::Create(const char* name, RenderResource* target_size)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::ReadbackBufferD3D12>(name, target_size);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture2D> Texture2D::Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture2DD3D12>(name, format, width, height, is_render_target, random_read_write_access);
#endif

#if RB_GRAPHICS_API_VULKAN
        case RenderAPI::Vulkan:
            return CreateShared<VK::Texture2DVK>(name, format, width, height, is_render_target, random_read_write_access);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture2D> Texture2D::Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t mips, bool is_render_target, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture2DD3D12>(name, format, width, height, mips, is_render_target, random_read_write_access);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture2D> Texture2D::Create(const char* name, const void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture2DD3D12>(name, data, data_size, format, width, height, is_render_target, random_read_write_access);
#endif
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture2D> Texture2D::Create(const char* name, const void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t mips, bool is_render_target, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture2DD3D12>(name, data, data_size, format, width, height, mips, is_render_target, random_read_write_access);
#endif
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture2D> Texture2D::Create(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture2DD3D12>(name, internal_resource, format, width, height, is_render_target, random_read_write_access);
#endif

#if RB_GRAPHICS_API_VULKAN
        case RenderAPI::Vulkan:
            return CreateShared<VK::Texture2DVK>(name, internal_resource, format, width, height, is_render_target, random_read_write_access);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture2D> Texture2D::Alias(const Shared<Texture2D>& original)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture2DD3D12>((D3D12::Texture2DD3D12*)original.get());
#endif

#if RB_GRAPHICS_API_VULKAN
        case RenderAPI::Vulkan:
            return CreateShared<VK::Texture2DVK>((VK::Texture2DVK*)original.get());
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture2DArray> Texture2DArray::Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t slices, bool is_render_target, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture2DArrayD3D12>(name, format, width, height, slices, is_render_target, random_read_write_access);
#endif

        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture2DArray> Texture2DArray::Alias(const Shared<Texture2DArray>& original)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture2DArrayD3D12>((D3D12::Texture2DArrayD3D12*)original.get());
#endif
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Shared<Texture3D> Texture3D::Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t depth, bool is_render_target, bool random_read_write_access)
    {
        switch (Renderer::GetAPI())
        {
#if RB_GRAPHICS_API_D3D12
        case RenderAPI::D3D12:
            return CreateShared<D3D12::Texture3DD3D12>(name, format, width, height, depth, is_render_target, random_read_write_access);
#endif
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }
}