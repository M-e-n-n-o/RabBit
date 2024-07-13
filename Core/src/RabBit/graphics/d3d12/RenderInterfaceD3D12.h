#pragma once

#include "graphics/RenderInterface.h"
#include "graphics/RenderResource.h"
#include "graphics/shaders/shared/Common.h"
#include "resource/BindlessDescriptorHeap.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class DeviceQueue;
	class GpuResource;
	class UploadAllocator;

	class GpuGuardD3D12 : public GpuGuard
	{
	public:
		GpuGuardD3D12(uint64_t fence_value, DeviceQueue* queue);

		bool IsFinishedRendering() override;
		void WaitUntilFinishedRendering() override;

	private:
		uint64_t				m_FenceValue;
		DeviceQueue*			m_Queue;

		friend class RenderInterfaceD3D12;
	};

	class RenderInterfaceD3D12 : public RenderInterface
	{
	public:
		RenderInterfaceD3D12(bool allow_only_copy_operations);
		~RenderInterfaceD3D12();

		void InvalidateState() override;

		// This method executes the command list and sets a new internal valid command list
		// Returns the execute ID (on which can be waited)
		Shared<GpuGuard> ExecuteInternal() override;
		void GpuWaitOn(GpuGuard* guard) override;

		void TransitionResource(RenderResource* resource, ResourceState state) override;
		void FlushResourceBarriers() override;

		void SetRenderTarget(RenderTargetBundle* bundle) override;

		void SetShaderResourceInput(RenderResource* resource, uint32_t slot) override;
		void ClearShaderResourceInput(uint32_t slot) override;

		void SetConstantShaderData(uint32_t slot, void* data, uint32_t data_size) override;

		void SetVertexShader(uint32_t shader_index) override;
		void SetPixelShader(uint32_t shader_index) override;

		void Clear(RenderResource* resource, const Math::Float4& color) override;

		void SetScissorRect(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom) override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void SetBlendMode(const BlendMode& mode) override;
		void SetCullMode(const CullMode& mode) override;
		void SetDepthMode(const DepthMode& mode) override;

		void SetIndexBuffer(RenderResource* index_resource) override;
		void SetVertexBuffer(RenderResource* vertex_resource, uint32_t slot) override;
		void SetVertexBuffers(RenderResource** vertex_resources, uint32_t resource_count, uint32_t start_slot) override;

		void CopyResource(RenderResource* src, RenderResource* dest) override;

		void UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size) override;

		void DrawInternal() override;
		void DispatchInternal() override;

		GPtr<ID3D12GraphicsCommandList2> GetCommandList() const { return m_CommandList; }

	private:
		void InternalCopy(GpuResource* src, GpuResource* dst, const RenderResourceType& primitive_type);
		void MarkResourceUsed(RenderResource* resource);
		void MarkResourceUsed(GpuResource* resource);

		void BindDescriptorHeaps();
		void BindDrawResources();
		void ClearDrawResources();

		void SetGraphicsPipelineState();
		void SetNewCommandList();

		bool								m_CopyOperationsOnly;
		DeviceQueue*						m_Queue;
		GPtr<ID3D12GraphicsCommandList2>	m_CommandList;

		struct RenderState
		{
			bool							psoDirty				= true;
			bool							rootSignatureDirty		= true;

			GPtr<ID3D12RootSignature>		rootSignature			= nullptr;
			D3D12_PRIMITIVE_TOPOLOGY_TYPE	vertexBufferType		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			uint32_t						vertexCountPerInstance	= 0;
			uint32_t						indexCountPerInstance	= 0;
			bool							scissorSet				= false;
			bool							viewportSet				= false;
			uint32_t						numRenderTargets		= 0;
			DXGI_FORMAT						rtvFormats[8];
			DXGI_FORMAT						dsvFormat;
			int32_t							vsShader				= -1;
			int32_t							psShader				= -1;
			bool							blendingSet				= false;
			D3D12_BLEND_DESC				blendDesc				= {};
			bool							rasterizerSet			= false;
			D3D12_RASTERIZER_DESC			rasterizerDesc			= {};
			bool							depthStencilSet			= false;
			D3D12_DEPTH_STENCIL_DESC		depthStencilDesc		= {};
			D3D12_GPU_VIRTUAL_ADDRESS		cbvAddresses[16];

			DescriptorHandle				tex2DsrvHandles[SHADER_TEX2D_SLOTS];
		};

		RenderState m_RenderState;

		struct UploadAllocatorPair
		{
			UploadAllocator* allocator;
			uint64_t fenceValue;
		};

		Queue<UploadAllocator*>		m_AvailableCBVAllocators;
		List<UploadAllocatorPair>	m_InFlightCBVAllocators;
		UploadAllocator*			m_CurrentCBVAllocator;
	};
}