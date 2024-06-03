#pragma once

#include "RabBitCommon.h"
#include "graphics/RenderResource.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	static D3D_PRIMITIVE_TOPOLOGY ConvertToD3D12Topology(const TopologyType& type)
	{
		switch (type)
		{
		case TopologyType::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		default:
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}
	}

    static DXGI_FORMAT ConvertToDXGIFormat(const RenderResourceFormat& format)
    {
        switch (format)
        {
        case(RenderResourceFormat::R32G32B32A32_TYPELESS):
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case(RenderResourceFormat::R32G32B32A32_FLOAT):
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case(RenderResourceFormat::R16G16B16A16_FLOAT):
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case(RenderResourceFormat::R32G32_FLOAT):
            return DXGI_FORMAT_R32G32_FLOAT;
        case(RenderResourceFormat::R32_UINT):
            return DXGI_FORMAT_R32_UINT;
        case(RenderResourceFormat::R8G8B8A8_TYPELESS):
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case(RenderResourceFormat::R8G8B8A8_SRGB):
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case(RenderResourceFormat::R8G8B8A8_UNORM):
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case(RenderResourceFormat::R11G11B10_FLOAT):
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case(RenderResourceFormat::R16G16_FLOAT):
            return DXGI_FORMAT_R16G16_FLOAT;
        case(RenderResourceFormat::R16G16_UINT):
            return DXGI_FORMAT_R16G16_UINT;
        case(RenderResourceFormat::R16_FLOAT):
            return DXGI_FORMAT_R16_FLOAT;
        case(RenderResourceFormat::R16_UINT):
            return DXGI_FORMAT_R16_UINT;
        case(RenderResourceFormat::R16_UNORM):
            return DXGI_FORMAT_R16_UNORM;
        case(RenderResourceFormat::R16_SNORM):
            return DXGI_FORMAT_R16_SNORM;
        case(RenderResourceFormat::R8_UNORM):
            return DXGI_FORMAT_R8_UNORM;
        case(RenderResourceFormat::R32_FLOAT):
            return DXGI_FORMAT_R32_FLOAT;
        case(RenderResourceFormat::R8_UINT):
            return DXGI_FORMAT_R8_UINT;
        case(RenderResourceFormat::Unkown):
            return DXGI_FORMAT_UNKNOWN;
        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Format not yet supported");
            return DXGI_FORMAT_UNKNOWN;
        }
    }

	static RenderResourceFormat ConvertToEngineFormat(const DXGI_FORMAT& format)
	{
        switch (format) 
        {
        case(DXGI_FORMAT_R32G32B32A32_TYPELESS):
            return RenderResourceFormat::R32G32B32A32_TYPELESS;
        case(DXGI_FORMAT_R32G32B32A32_FLOAT):
            return RenderResourceFormat::R32G32B32A32_FLOAT;
        case(DXGI_FORMAT_R16G16B16A16_FLOAT):
            return RenderResourceFormat::R16G16B16A16_FLOAT;
        case(DXGI_FORMAT_R32G32_FLOAT):
            return RenderResourceFormat::R32G32_FLOAT;
        case(DXGI_FORMAT_R32_UINT):
            return RenderResourceFormat::R32_UINT;
        case(DXGI_FORMAT_R32_FLOAT):
            return RenderResourceFormat::R32_FLOAT;
        case(DXGI_FORMAT_R8G8B8A8_TYPELESS):
            return RenderResourceFormat::R8G8B8A8_TYPELESS;
        case(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
            return RenderResourceFormat::R8G8B8A8_SRGB;
        case(DXGI_FORMAT_R8G8B8A8_UNORM):
            return RenderResourceFormat::R8G8B8A8_UNORM;
        case(DXGI_FORMAT_R11G11B10_FLOAT):
            return RenderResourceFormat::R11G11B10_FLOAT;
        case(DXGI_FORMAT_R16G16_FLOAT):
            return RenderResourceFormat::R16G16_FLOAT;
        case(DXGI_FORMAT_R16G16_UINT):
            return RenderResourceFormat::R16G16_UINT;
        case(DXGI_FORMAT_R16_FLOAT):
            return RenderResourceFormat::R16_FLOAT;
        case(DXGI_FORMAT_R16_UINT):
            return RenderResourceFormat::R16_UINT;
        case(DXGI_FORMAT_R16_UNORM):
            return RenderResourceFormat::R16_UNORM;
        case(DXGI_FORMAT_R16_SNORM):
            return RenderResourceFormat::R16_SNORM;
        case(DXGI_FORMAT_R8_UNORM):
            return RenderResourceFormat::R8_UNORM;
        case(DXGI_FORMAT_R8_UINT):
            return RenderResourceFormat::R8_UINT;
        case(DXGI_FORMAT_UNKNOWN):
            return RenderResourceFormat::Unkown;
        default:
            RB_LOG_WARN(LOGTAG_GRAPHICS, "Format not yet supported");
            return RenderResourceFormat::Unkown;
        }
	}
}