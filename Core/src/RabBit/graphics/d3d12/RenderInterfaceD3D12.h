#pragma once

#include "graphics/RenderInterface.h"

namespace RB::Graphics::D3D12
{
	class RenderInterfaceD3D12 : public RenderInterface
	{
	public:
		void Flush() override;

		void TransitionResource(RenderResource* resource, ResourceState state) override;
		void FlushBarriers() override;

		void EnableWireframeMode() override;
		void DisableWireframeMode() override;

		void SetCullMode() override;

		void SetVertexShader(uint32_t shader_index) override;
		void SetPixelShader(uint32_t shader_index) override;

		void Draw() override;
	};
}