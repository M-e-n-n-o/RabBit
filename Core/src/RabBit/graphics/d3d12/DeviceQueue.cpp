#include "RabBitCommon.h"
#include "DeviceQueue.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
	DeviceQueue::DeviceQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority, D3D12_COMMAND_QUEUE_FLAGS flags)
		: m_Type(type)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = type;
		desc.Priority = priority;
		desc.Flags = flags;
		desc.NodeMask = 0;

		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)), "Could not create command queue");

		switch (type)
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:  m_CommandQueue->SetName(L"Direct Queue"); break;
		case D3D12_COMMAND_LIST_TYPE_COMPUTE: m_CommandQueue->SetName(L"Compute Queue"); break;
		case D3D12_COMMAND_LIST_TYPE_COPY:    m_CommandQueue->SetName(L"Copy Queue"); break;
		}

		CreateFence();
	}

	DeviceQueue::~DeviceQueue()
	{
		CpuWaitUntilIdle();

		CloseHandle(m_FenceEventHandle);
	}

	void DeviceQueue::CpuWaitUntilIdle(uint64_t max_duration_ms)
	{
		uint64_t fence_value_for_signal = SignalFence();
		CpuWaitForFenceValue(fence_value_for_signal, max_duration_ms);
	}

	GPtr<ID3D12GraphicsCommandList2> DeviceQueue::GetCommandList()
	{
		GPtr< ID3D12CommandAllocator> command_allocator = nullptr;
		GPtr<ID3D12GraphicsCommandList2> command_list = nullptr;

		UpdateAvailableCommandAllocators();

		if (m_AvailableCommandAllocators.empty())
		{
			RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateCommandAllocator(m_Type, IID_PPV_ARGS(&command_allocator)), 
				"Could not create command allocator");
		}
		else
		{
			command_allocator = m_AvailableCommandAllocators.front();
			m_AvailableCommandAllocators.pop();

			RB_ASSERT_FATAL_RELEASE_D3D(command_allocator->Reset(), "Could not reset command allocator");
		}

		if (m_CommandListQueue.empty())
		{
			RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateCommandList(0, m_Type, command_allocator.Get(), nullptr, IID_PPV_ARGS(&command_list)), 
				"Could not create command list");
		}
		else
		{
			command_list = m_CommandListQueue.front();
			m_CommandListQueue.pop();

			RB_ASSERT_FATAL_RELEASE_D3D(command_list->Reset(command_allocator.Get(), nullptr), 
				"Could not reset command list");
		}

		// Link the command allocator to the command list to retrieve back in the ExecuteCommandLists
		RB_ASSERT_FATAL_RELEASE_D3D(command_list->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), command_allocator.Get()), 
			"Could not set the command allocator as private data in the command list");

		return command_list;
	}

	void DeviceQueue::UpdateAvailableCommandAllocators()
	{
		auto itr = m_RunningCommandAllocators.begin();
		while (itr != m_RunningCommandAllocators.end())
		{
			if (IsFenceReached(itr->fenceValue))
			{
				m_AvailableCommandAllocators.push(itr->commandAllocator);
				itr = m_RunningCommandAllocators.erase(itr);
			}
			else
			{
				++itr;
			}
		}
	}

	uint64_t DeviceQueue::ExecuteCommandList(GPtr<ID3D12GraphicsCommandList2> command_list)
	{
		GPtr<ID3D12GraphicsCommandList2> ptr[] = { command_list };

		return ExecuteCommandLists(1, ptr);
	}

	uint64_t DeviceQueue::ExecuteCommandLists(uint32_t num_command_lists, GPtr<ID3D12GraphicsCommandList2>* command_lists)
	{
		ID3D12CommandList** lists = (ID3D12CommandList**)alloca(sizeof(ID3D12CommandList*) * num_command_lists);
		ID3D12CommandAllocator** allocators = (ID3D12CommandAllocator**)alloca(sizeof(ID3D12CommandAllocator*) * num_command_lists);

		for (uint32_t i = 0; i < num_command_lists; ++i)
		{
			lists[i] = command_lists[i].Get();

			// Close the command lists
			RB_ASSERT_FATAL_RELEASE_D3D(command_lists[i]->Close(),
				"Could not close the command list");

			// Retrieve the command allocators
			UINT data_size = sizeof(allocators[i]);
			RB_ASSERT_FATAL_RELEASE_D3D(command_lists[i]->GetPrivateData(__uuidof(ID3D12CommandAllocator), &data_size, &allocators[i]),
				"Could not retrieve the command allocator from the command list");
		}

		// Execute command lists
		m_CommandQueue->ExecuteCommandLists(num_command_lists, lists);
		uint64_t fence_value = SignalFence();

		for (uint32_t i = 0; i < num_command_lists; ++i)
		{
			// Set the command allocators as busy
			m_RunningCommandAllocators.push_back({ fence_value, allocators[i] });
			allocators[i]->Release();
			
			m_CommandListQueue.push(command_lists[i]);
		}


		return fence_value;
	}
	
	void DeviceQueue::CreateFence()
	{
		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)), "Could not create fence");

		m_FenceEventHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, m_FenceEventHandle, "Could not create fence event");

		m_FenceValue = 0;
	}
	
	uint64_t DeviceQueue::SignalFence()
	{
		uint64_t fence_value_for_signal = ++m_FenceValue;
		RB_ASSERT_FATAL_RELEASE_D3D(m_CommandQueue->Signal(m_Fence.Get(), fence_value_for_signal), "Could not signal the command queue");
		return fence_value_for_signal;
	}

	bool DeviceQueue::IsFenceReached(uint64_t fence_value)
	{
		return m_Fence->GetCompletedValue() >= fence_value;
	}

	void DeviceQueue::CpuWaitForFenceValue(uint64_t fence_value, uint64_t max_duration_ms)
	{
		if (IsFenceReached(fence_value))
		{
			return;
		}
		
		RB_ASSERT_FATAL_RELEASE_D3D(m_Fence->SetEventOnCompletion(fence_value, m_FenceEventHandle), "Could not hang event on fence");
		::WaitForSingleObject(m_FenceEventHandle, static_cast<DWORD>(max_duration_ms));
	}

	void DeviceQueue::GpuWaitForFenceValue(uint64_t fence_value)
	{
		GpuWaitForFenceValue(m_Fence, fence_value);
	}
	
	void DeviceQueue::GpuWaitForFenceValue(GPtr<ID3D12Fence> fence, uint64_t fence_value)
	{
		m_CommandQueue->Wait(fence.Get(), fence_value);
	}
}