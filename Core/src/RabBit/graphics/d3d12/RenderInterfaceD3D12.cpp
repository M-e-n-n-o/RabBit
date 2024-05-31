#include "RabBitCommon.h"
#include "RenderInterfaceD3D12.h"
#include "DeviceQueue.h"
#include "resource/ResourceManager.h"
#include "resource/RenderResourceD3D12.h"
#include "resource/ResourceStateManager.h"
#include "UtilsD3D12.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
	RIExecutionGuardD3D12::RIExecutionGuardD3D12(uint64_t fence_value, DeviceQueue* queue)
		: m_FenceValue(fence_value)
		, m_Queue(queue)
	{
	}

	bool RIExecutionGuardD3D12::IsFinishedRendering()
	{
		return m_Queue->IsFenceReached(m_FenceValue);
	}

	void RIExecutionGuardD3D12::WaitUntilFinishedRendering()
	{
		m_Queue->CpuWaitForFenceValue(m_FenceValue);
	}

	// ---------------------------------------------------------------------------
	//								RIExecutionGuard
	// ---------------------------------------------------------------------------

	RenderInterfaceD3D12::RenderInterfaceD3D12(bool allow_only_copy_operations)
		: m_CopyOperationsOnly(allow_only_copy_operations)
		, m_RenderState()
	{
		if (allow_only_copy_operations)
			m_Queue = g_GraphicsDevice->GetComputeQueue();
		else
			m_Queue = g_GraphicsDevice->GetGraphicsQueue();

		SetNewCommandList();
	}

	void RenderInterfaceD3D12::InvalidateState()
	{
		m_RenderState = {};
	}

	Shared<RIExecutionGuard> RenderInterfaceD3D12::ExecuteInternal()
	{
		uint64_t fence_value = m_Queue->ExecuteCommandList(m_CommandList);

		g_ResourceManager->OnCommandListExecute(m_Queue, fence_value);

		InvalidateState();
		SetNewCommandList();

		return CreateShared<RIExecutionGuardD3D12>(fence_value, m_Queue);
	}

	void RenderInterfaceD3D12::GpuWaitOn(RIExecutionGuard* guard)
	{
		RIExecutionGuardD3D12* d3d_guard = (RIExecutionGuardD3D12*) guard;
		m_Queue->GpuWaitForFenceValue(d3d_guard->m_Queue->GetFence(), d3d_guard->m_FenceValue);
	}

	void RenderInterfaceD3D12::FlushResourceBarriers()
	{
		g_ResourceStateManager->FlushPendingTransitions(m_CommandList.Get());
	}

	void RenderInterfaceD3D12::SetNewCommandList()
	{
		m_CommandList = m_Queue->GetCommandList();
	}

	void RenderInterfaceD3D12::Clear(RenderResource* resource, const Math::Float4& color)
	{
		FLOAT clear_color[] = { color.r, color.g, color.b, color.a };
		m_CommandList->ClearRenderTargetView(((D3D12::Texture2DD3D12*)resource)->GetCpuHandle(), clear_color, 0, nullptr);
	}

	void RenderInterfaceD3D12::SetScissorRect(const Math::Int4& scissor_rect)
	{
		D3D12_RECT rect = {};
		rect.left	= scissor_rect.x;
		rect.right	= scissor_rect.y;
		rect.top	= scissor_rect.z;
		rect.bottom = scissor_rect.w;

		m_CommandList->RSSetScissorRects(1, &rect);

		m_RenderState.scissorSetExplicitly = true;
	}

	void RenderInterfaceD3D12::SetVertexBuffer(uint32_t slot, RenderResource* vertex_resource)
	{
		RenderResource* resources[] = { vertex_resource };
		SetVertexBuffers(slot, resources, 1);
	}

	void RenderInterfaceD3D12::SetVertexBuffers(uint32_t start_slot, RenderResource** vertex_resources, uint32_t resource_count)
	{
		D3D12_VERTEX_BUFFER_VIEW* views = (D3D12_VERTEX_BUFFER_VIEW*)ALLOC_STACK(sizeof(D3D12_VERTEX_BUFFER_VIEW) * resource_count);

		D3D_PRIMITIVE_TOPOLOGY type = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

		for (uint32_t res_idx = 0; res_idx < resource_count; ++res_idx)
		{
			if (vertex_resources[res_idx]->GetType() != RenderResourceType::VertexBuffer)
			{
				RB_LOG_ERROR(LOGTAG_GRAPHICS, "Passed in resource %d was not a vertex buffer", res_idx);
				return;
			}

			if (!ValidateResource(vertex_resources[res_idx]))
			{
				RB_LOG_WARN(LOGTAG_GRAPHICS, "Vertex buffer was not valid yet when calling SetVertexBuffer, so it probably does not contain any data right now");
			}

			VertexBufferD3D12* vbo = (VertexBufferD3D12*)vertex_resources[res_idx];

			views[res_idx] = vbo->GetView();

			if (type != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
			{
				RB_ASSERT_FATAL(LOGTAG_GRAPHICS, type == ConvertToD3D12Topology(vbo->GetTopologyType()), "The topology types of the passed in vertex buffers do not match");
			}
			else
			{
				type = ConvertToD3D12Topology(vbo->GetTopologyType());
			}
		}

		m_CommandList->IASetVertexBuffers(start_slot, resource_count, views);
		m_CommandList->IASetPrimitiveTopology(type);

		m_RenderState.vertexBufferSet = true;
	}

	void RenderInterfaceD3D12::CopyResource(RenderResource* src, RenderResource* dest)
	{
		if (src->GetType() != dest->GetType())
		{
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Can not copy resource as the typed do not match");
			return;
		}

		GpuResource* src_res = (GpuResource*)src->GetNativeResource();
		GpuResource* dest_res = (GpuResource*)dest->GetNativeResource();

		switch (src->GetPrimitiveType())
		{
		case RenderResourceType::Buffer:
		{
			InternalCopyBuffer(src_res, dest_res);
		}
		break;

		default:
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}
	}

	void RenderInterfaceD3D12::UploadDataToResource(RenderResource* resource, void* data, uint64_t data_size)
	{
		switch (resource->GetPrimitiveType())
		{
		case RenderResourceType::Buffer:
		{
			GpuResource* upload_res = new GpuResource();
			g_ResourceManager->ScheduleCreateUploadResource(upload_res, "Upload resource", data_size);
			
			char* mapped_mem;
			RB_ASSERT_FATAL_D3D(upload_res->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mapped_mem)), "Could not map the upload resource");

			memcpy(mapped_mem, data, data_size);

			// Unmapping is unnecessary
			//upload_res->Unmap(0, nullptr);

			InternalCopyBuffer(upload_res, (GpuResource*)resource->GetNativeResource());

			delete upload_res;
		}
		break;

		default:
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}
	}

	void RenderInterfaceD3D12::DrawInternal()
	{
	}

	void RenderInterfaceD3D12::DispatchInternal()
	{
	}

	void RenderInterfaceD3D12::InternalCopyBuffer(GpuResource* src, GpuResource* dest)
	{
		g_ResourceStateManager->TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
		g_ResourceStateManager->TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
		FlushResourceBarriers();

		m_CommandList->CopyResource(dest->GetResource().Get(), src->GetResource().Get());
	}

	bool RenderInterfaceD3D12::ValidateResource(RenderResource* res)
	{
		GpuResource* gpu_res = (GpuResource*)res->GetNativeResource();

		if (gpu_res->IsValid())
		{
			return true;
		}

		return false;
	}
}