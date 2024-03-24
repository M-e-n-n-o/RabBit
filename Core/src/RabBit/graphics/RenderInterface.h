#pragma once

#include "ResourceState.h"

namespace RB::Math
{
	struct Int4;
}

namespace RB::Graphics
{
	#define INTERMEDIATE_EXECUTE_THRESHOLD 500

	class RenderResource;
	class RenderInterface;

	class RIExecutionGuard
	{
	public:
		virtual ~RIExecutionGuard() = default;

		virtual bool IsFinishedRendering() = 0;
		virtual void WaitUntilFinishedRendering() = 0;

	protected:
		RIExecutionGuard() = default;
	};

	class RenderInterface
	{
	public:
		virtual ~RenderInterface() = default;

		// Call after leaving from a third-party library that issues render commands
		virtual void InvalidateState() = 0;

		Shared<RIExecutionGuard> ExecuteOnGpu();

		virtual void GpuWaitOn(RIExecutionGuard* guard) = 0;

		//virtual void TransitionResource(RenderResource* resource, ResourceState state) = 0;
		virtual void FlushResourceBarriers() = 0;

		//virtual void SetVertexShader(uint32_t shader_index) = 0;
		//virtual void SetPixelShader(uint32_t shader_index) = 0;

		virtual void SetScissorRect(const Math::Int4& scissor_rect) = 0;

		// ALSO SET THE VIEWPORT (AND SCISSOR IN THIS CALL IF THEY HAVE NOT YET BEEN EXPLICITELY SET)!!!
		//virtual void SetRenderTarget(uint32_t slot, RenderResource* render_target) = 0;
		//virtual void SetRenderTargets(uint32_t start_slot, RenderResource** render_targets) = 0;

		virtual void SetVertexBuffer(uint32_t slot, RenderResource* vertex_resource) = 0;
		virtual void SetVertexBuffers(uint32_t start_slot, RenderResource** vertex_resources, uint32_t resource_count) = 0;

		virtual void UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size) = 0;

		virtual void CopyResource(RenderResource* src, RenderResource* dest) = 0;

		//virtual void SetShaderInput() = 0;

		void Draw();
		void Dispatch();

		static RenderInterface* Create(bool allow_only_copy_operations);

	protected:
		RenderInterface() = default;

		bool NeedsIntermediateExecute();

		virtual Shared<RIExecutionGuard> ExecuteInternal() = 0;
		virtual void DrawInternal() = 0;
		virtual void DispatchInternal() = 0;

		uint32_t m_TotalDraws = 0;
	};
}