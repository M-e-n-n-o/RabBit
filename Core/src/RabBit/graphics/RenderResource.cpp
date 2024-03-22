#include "RabBitCommon.h"
#include "RenderResource.h"
#include "Renderer.h"
#include "d3d12/resource/RenderResourceD3D12.h"

namespace RB::Graphics
{
	VertexBuffer* VertexBuffer::Create(const char* name, void* data, uint64_t data_size)
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
			return new D3D12::VertexBufferD3D12(name, data, data_size);
		default:
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Did not yet implement the Renderer for the set graphics API");
			break;
		}

		return nullptr;
	}
}