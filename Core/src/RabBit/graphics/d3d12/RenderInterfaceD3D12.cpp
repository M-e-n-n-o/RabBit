#include "RabBitCommon.h"
#include "RenderInterfaceD3D12.h"
#include "DeviceQueue.h"
#include "resource/ResourceManager.h"
#include "graphics/RenderResource.h"
#include "resource/ResourceStateManager.h"
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
	{
		if (allow_only_copy_operations)
			m_Queue = g_GraphicsDevice->GetComputeQueue();
		else
			m_Queue = g_GraphicsDevice->GetGraphicsQueue();

		SetNewCommandList();
	}

	void RenderInterfaceD3D12::InvalidateState()
	{
		// TODO
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

	void RenderInterfaceD3D12::SetVertexBuffer(RenderResource* vertex_resource)
	{
		if (vertex_resource->GetType() != RenderResourceType::VertexBuffer)
		{
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "The passed in resource was not a vertex buffer");
			return;
		}

		if (!ValidateResource(vertex_resource))
		{
			RB_LOG_WARN(LOGTAG_GRAPHICS, "Vertex buffer was not valid when calling SetVertexBuffer");
		}

		// TODO Get the input layout from the resource to determine on fill in the correct parameters
		m_CommandList->IASetVertexBuffers();
	}

	void RenderInterfaceD3D12::CopyResource(RenderResource* src, RenderResource* dest)
	{
		ValidateResource(dest);

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
		ValidateResource(resource);

		switch (resource->GetPrimitiveType())
		{
		case RenderResourceType::Buffer:
		{
			GpuResource* upload_res = new GpuResource();
			g_ResourceManager->CreateUploadResource(upload_res, L"Upload resource", data_size);
			
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

	void RenderInterfaceD3D12::InternalCopyBuffer(GpuResource* src, GpuResource* dest)
	{
		g_ResourceStateManager->TransitionResource(src->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
		g_ResourceStateManager->TransitionResource(dest->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST);
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

		wchar_t* wchar = new wchar_t[strlen(res->GetName()) + 1];
		CharToWchar(res->GetName(), wchar);

		switch (res->GetType())
		{
		case RenderResourceType::VertexBuffer: g_ResourceManager->CreateVertexResource(gpu_res, wchar, res->GetSize()); break;

		case RenderResourceType::Unknown:
		default:
			RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Not yet implemented");
			break;
		}
		
		delete[] wchar;

		return false;
	}
}