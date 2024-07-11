#include "RabBitCommon.h"
#include "ShaderSystem.h"
#include "Renderer.h"
#include "d3d12/ShaderSystemD3D12.h"

namespace RB::Graphics
{
	ShaderSystem* ShaderSystem::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
			return new D3D12::ShaderSystemD3D12();
		default:
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Did not yet implement the ShaderSystem for the set graphics API");
			break;
		}

		return nullptr;
	}
}