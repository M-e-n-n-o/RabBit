#include "RabBitCommon.h"
#include "RenderInterfaceD3D12.h"

namespace RB::Graphics::D3D12
{
	RenderInterfaceD3D12::RenderInterfaceD3D12()
	{
	}

	void RenderInterfaceD3D12::SetCommandList(ID3D12GraphicsCommandList2* command_list)
	{
	}

	void RenderInterfaceD3D12::InvalidateState()
	{
	}

	uint32_t RenderInterfaceD3D12::ExecuteInternal()
	{
		return 0;
	}

	void RenderInterfaceD3D12::SyncCpuWithGpu(uint32_t id)
	{
	}

	void RenderInterfaceD3D12::SyncGpuWithGpu(uint32_t id)
	{
	}

	void RenderInterfaceD3D12::TransitionResource(RenderResource* resource, ResourceState state)
	{
	}

	void RenderInterfaceD3D12::FlushBarriers()
	{
	}

	void RenderInterfaceD3D12::EnableWireframeMode()
	{
	}

	void RenderInterfaceD3D12::DisableWireframeMode()
	{
	}

	void RenderInterfaceD3D12::SetCullMode()
	{
	}

	void RenderInterfaceD3D12::SetVertexShader(uint32_t shader_index)
	{
	}

	void RenderInterfaceD3D12::SetPixelShader(uint32_t shader_index)
	{
	}

	void RenderInterfaceD3D12::DrawInternal()
	{
	}

	void RenderInterfaceD3D12::DispatchInternal()
	{
	}
}