#include "RabBitCommon.h"
#include "RenderResource.h"
#include "Renderer.h"
#include "d3d12/resource/RenderResourceD3D12.h"

namespace RB::Graphics
{
    uint32_t GetElementSizeFromFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case(RenderResourceFormat::R32G32B32A32_TYPELESS):
        case(RenderResourceFormat::R32G32B32A32_FLOAT):
            return 16;
        case(RenderResourceFormat::R32G32_FLOAT):
        case(RenderResourceFormat::R16G16B16A16_FLOAT):
            return 8;
        case(RenderResourceFormat::R32_UINT):
        case(RenderResourceFormat::R8G8B8A8_TYPELESS):
        case(RenderResourceFormat::R8G8B8A8_SRGB):
        case(RenderResourceFormat::R8G8B8A8_UNORM):
        case(RenderResourceFormat::R16G16_FLOAT):
        case(RenderResourceFormat::R16G16_UINT):
        case(RenderResourceFormat::R32_FLOAT):
        case(RenderResourceFormat::D32_FLOAT):
            return 4;
        case(RenderResourceFormat::R16_FLOAT):
        case(RenderResourceFormat::R16_UINT):
        case(RenderResourceFormat::R16_UNORM):
        case(RenderResourceFormat::R16_SNORM):
        case(RenderResourceFormat::D16_UNORM):
            return 2;
        case(RenderResourceFormat::R8_UNORM):
        case(RenderResourceFormat::R8_UINT):
            return 1;
        case(RenderResourceFormat::Unkown):
            return 0;
        case(RenderResourceFormat::R11G11B10_FLOAT):
        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Format not yet supported");
            return 0;
        }
    }

    bool IsDepthFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case RenderResourceFormat::D32_FLOAT:
        case RenderResourceFormat::D16_UNORM:
            return true;

        case RenderResourceFormat::R32G32B32A32_TYPELESS:
        case RenderResourceFormat::R32G32B32A32_FLOAT:
        case RenderResourceFormat::R16G16B16A16_FLOAT:
        case RenderResourceFormat::R32G32_FLOAT:
        case RenderResourceFormat::R8_UINT:
        case RenderResourceFormat::R32_UINT:
        case RenderResourceFormat::R8G8B8A8_TYPELESS:
        case RenderResourceFormat::R8G8B8A8_UNORM:
        case RenderResourceFormat::R8G8B8A8_SRGB:
        case RenderResourceFormat::R11G11B10_FLOAT:
        case RenderResourceFormat::R16G16_FLOAT:
        case RenderResourceFormat::R16G16_UINT:
        case RenderResourceFormat::R16_FLOAT:
        case RenderResourceFormat::R16_UINT:
        case RenderResourceFormat::R16_UNORM:
        case RenderResourceFormat::R16_SNORM:
        case RenderResourceFormat::R8_UNORM:
        case RenderResourceFormat::R32_FLOAT:
            return false;
        default:
            return false;
        }
    }

    float Texture2D::GetAspectRatio() const
    {
        return (float)GetWidth() / (float)GetHeight();
    }

    RenderResourceType RenderResource::GetPrimitiveType() const
    {
        uint32_t last_primitive = (uint32_t)RenderResourceType::kLastPrimitiveType;
        uint32_t and_value = last_primitive + (last_primitive - 1);

        uint32_t primitive_type = (uint32_t)m_Type & and_value;

        return (RenderResourceType)primitive_type;
    }

    VertexBuffer* VertexBuffer::Create(const char* name, const TopologyType& type, void* data, uint32_t vertex_size, uint64_t data_size)
    {
        switch (Renderer::GetAPI())
        {
        case RenderAPI::D3D12:
            return new D3D12::VertexBufferD3D12(name, type, data, vertex_size, data_size);
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    IndexBuffer* IndexBuffer::Create(const char* name, uint32_t* data, uint64_t data_size)
    {
        switch (Renderer::GetAPI())
        {
        case RenderAPI::D3D12:
            return new D3D12::IndexBufferD3D12(name, data, data_size);
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Texture2D* Texture2D::Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space)
    {
        switch (Renderer::GetAPI())
        {
        case RenderAPI::D3D12:
            return new D3D12::Texture2DD3D12(name, format, width, height, is_render_target, random_read_write_access, color_space);
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Texture2D* Texture2D::Create(const char* name, void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space)
    {
        switch (Renderer::GetAPI())
        {
        case RenderAPI::D3D12:
            return new D3D12::Texture2DD3D12(name, data, data_size, format, width, height, is_render_target, random_read_write_access, color_space);
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }

    Texture2D* Texture2D::Create(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access, TextureColorSpace color_space)
    {
        switch (Renderer::GetAPI())
        {
        case RenderAPI::D3D12:
            return new D3D12::Texture2DD3D12(name, internal_resource, format, width, height, is_render_target, random_read_write_access, color_space);
        default:
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Not yet implemented");
            break;
        }

        return nullptr;
    }
}