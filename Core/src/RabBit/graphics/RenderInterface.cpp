#include "RabBitCommon.h"
#include "Renderer.h"
#include "RenderInterface.h"

#include "d3d12/RenderInterfaceD3D12.h"

namespace RB::Graphics
{
	RenderInterface* RenderInterface::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
			return new D3D12::RenderInterfaceD3D12();
		default:
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Did not yet implement the render interface for the set graphics API");
			break;
		}
	}
}