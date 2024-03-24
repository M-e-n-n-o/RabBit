#pragma once

#include "graphics/RenderInterface.h"

namespace RB::Graphics::D3D12
{
	class DeviceQueue;
	class GpuResource;

	class RIExecutionGuardD3D12 : public RIExecutionGuard
	{
	public:
		RIExecutionGuardD3D12(uint64_t fence_value, DeviceQueue* queue);

		virtual bool IsFinishedRendering() override;
		virtual void WaitUntilFinishedRendering() override;

	private:
		uint64_t m_FenceValue;
		DeviceQueue* m_Queue;

		friend class RenderInterfaceD3D12;
	};

	class RenderInterfaceD3D12 : public RenderInterface
	{
	public:
		RenderInterfaceD3D12(bool allow_only_copy_operations);

		// The render interface does not own the command list, so it does not get a shared pointer
		//void SetCommandList(ID3D12GraphicsCommandList2* command_list);

		void InvalidateState() override;

		// This method executes the command list and sets a new internal valid command list
		// Returns the execute ID (on which can be waited)
		Shared<RIExecutionGuard> ExecuteInternal() override;
		void GpuWaitOn(RIExecutionGuard* guard) override;

		//void TransitionResource(RenderResource* resource, ResourceState state) override;
		void FlushResourceBarriers() override;

		//void SetVertexShader(uint32_t shader_index) override;
		//void SetPixelShader(uint32_t shader_index) override;

		void SetScissorRect(const Math::Int4& scissor_rect) override;

		void SetVertexBuffer(uint32_t slot, RenderResource* vertex_resource) override;
		void SetVertexBuffers(uint32_t start_slot, RenderResource** vertex_resources, uint32_t resource_count) override;

		void CopyResource(RenderResource* src, RenderResource* dest) override;

		void UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size) override;

		// CHECK IF ALL ESSENTIAL STATES ARE SET!!!
		void DrawInternal() override;
		void DispatchInternal() override;

	private:
		bool ValidateResource(RenderResource* res);
		void SetNewCommandList();
		void InternalCopyBuffer(GpuResource* src, GpuResource* dest);

		bool								m_CopyOperationsOnly;
		DeviceQueue*						m_Queue;
		GPtr<ID3D12GraphicsCommandList2>	m_CommandList;

		struct RenderState
		{
			bool vertexBufferSet		= false;
			bool scissorSetExplicitly	= false;
		};

		RenderState m_RenderState;
	};
}