#pragma once

#include "graphics/RenderInterface.h"
#include "DeviceQueue.h"

namespace RB::Graphics::D3D12
{
	class RenderInterfaceD3D12 : public RenderInterface
	{
	public:
		RenderInterfaceD3D12();

		// The render interface does not own the command list, so it does not get a shared pointer
		void SetCommandList(ID3D12GraphicsCommandList2* command_list);

		void InvalidateState() override;

		// This method executes the command list and sets a new internal valid command list
		// Returns the execute ID (on which can be waited)
		uint32_t ExecuteInternal() override;
		void SyncCpuWithGpu(uint32_t id) override;
		void SyncGpuWithGpu(uint32_t id) override;

		void TransitionResource(RenderResource* resource, ResourceState state) override;
		void FlushBarriers() override;

		void EnableWireframeMode() override;
		void DisableWireframeMode() override;

		void SetCullMode() override;

		void SetVertexShader(uint32_t shader_index) override;
		void SetPixelShader(uint32_t shader_index) override;

		void DrawInternal() override;
		void DispatchInternal() override;

	private:

		ID3D12GraphicsCommandList2* m_CommandList;
	};
}