#pragma once

// If you get an error that this file cannot be found, run the ShaderCompiler project for the graphics API you are using
#include "codeGen/ShaderDefines.h"

namespace RB::Math
{
	struct Float4;
}

namespace RB::Graphics
{
	#define INTERMEDIATE_EXECUTE_THRESHOLD 350

	class RenderResource;
	class RenderInterface;
	class RenderTargetBundle;
	enum class ResourceState;

	enum class BlendMode
	{
		None
	};

	enum class CullMode
	{
		None,
		Front,
		Back
	};

	enum class DepthMode
	{
		Disabled,
	};

	class GpuGuard
	{
	public:
		virtual ~GpuGuard() = default;

		virtual bool IsFinishedRendering() = 0;
		virtual void WaitUntilFinishedRendering() = 0;

	protected:
		GpuGuard() = default;
	};

	class RenderInterface
	{
	public:
		virtual ~RenderInterface() = default;

		// Call after leaving from a third-party library that issues render commands
		virtual void InvalidateState() = 0;

		Shared<GpuGuard> ExecuteOnGpu();

		virtual void GpuWaitOn(GpuGuard* guard) = 0;

		// You should normally not have to manually transition resources, this will be done automatically
		virtual void TransitionResource(RenderResource* resource, ResourceState state) = 0;
		virtual void FlushResourceBarriers() = 0;

		virtual void SetConstantShaderData(uint32_t slot, void* data, uint32_t data_size) = 0;

		virtual void SetVertexShader(uint32_t shader_index) = 0;
		virtual void SetPixelShader(uint32_t shader_index) = 0;

		virtual void Clear(RenderResource* resource, const Math::Float4& color) = 0;

		virtual void SetScissorRect(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom) = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void SetRenderTarget(RenderTargetBundle* bundle) = 0;

		virtual void SetBlendMode(const BlendMode& mode) = 0;
		virtual void SetCullMode(const CullMode& mode) = 0;
		virtual void SetDepthMode(const DepthMode& mode) = 0;

		virtual void SetIndexBuffer(RenderResource* index_resource) = 0;
		virtual void SetVertexBuffer(RenderResource* vertex_resource, uint32_t slot = 0) = 0;
		virtual void SetVertexBuffers(RenderResource** vertex_resources, uint32_t resource_count, uint32_t start_slot = 0) = 0;

		virtual void UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size) = 0;

		virtual void CopyResource(RenderResource* src, RenderResource* dest) = 0;

		//virtual void SetShaderInput() = 0;

		void Draw();
		void Dispatch();

		static RenderInterface* Create(bool allow_only_copy_operations);

	protected:
		RenderInterface() = default;

		bool NeedsIntermediateExecute();

		virtual Shared<GpuGuard> ExecuteInternal() = 0;
		virtual void DrawInternal() = 0;
		virtual void DispatchInternal() = 0;

		uint32_t m_TotalDraws = 0;
	};
}