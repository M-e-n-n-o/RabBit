#pragma once

#include "ResourceState.h"

namespace RB::Graphics
{
	class RenderResource;

	class RenderInterface
	{
	public:
		virtual ~RenderInterface() = default;

		// Call after leaving from a third-party library that issues render commands
		//virtual void InvalidateState() = 0;

		virtual void Flush() = 0;

		virtual void TransitionResource(RenderResource* resource, ResourceState state) = 0;
		virtual void FlushBarriers() = 0;

		virtual void EnableWireframeMode() = 0;
		virtual void DisableWireframeMode() = 0;

		virtual void SetCullMode() = 0;

		virtual void SetVertexShader(uint32_t shader_index) = 0;
		virtual void SetPixelShader(uint32_t shader_index) = 0;

		virtual void Draw() = 0;

		//virtual void CopyResource(Resource* src, Resource* dest) = 0;

		//virtual void SetShaderInput() = 0;

		static RenderInterface* Create();
	};
}