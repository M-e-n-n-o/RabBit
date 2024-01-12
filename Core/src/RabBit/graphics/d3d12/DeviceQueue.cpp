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

		CreateFence();
	}

	DeviceQueue::~DeviceQueue()
	{
		WaitUntilEmpty();
		UpdateAvailableCommandLists();
		
		for (uint32_t i = 0; i < m_AvailableCommandLists.size(); ++i)
		{
			delete m_AvailableCommandLists[i];
		}

		CloseHandle(m_FenceEventHandle);
	}

	void DeviceQueue::WaitUntilEmpty(uint64_t max_duration_ms)
	{
		uint64_t fence_value_for_signal = SignalFence();
		WaitForFenceValue(fence_value_for_signal, max_duration_ms);
	}

	CommandList* DeviceQueue::GetCommandList()
	{
		CommandList* command_list = nullptr;

		UpdateAvailableCommandLists();

		if (m_AvailableCommandLists.empty())
		{
			GPtr<ID3D12CommandAllocator> d3d_command_allocator;
			GPtr<ID3D12GraphicsCommandList2> d3d_command_list;

			RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateCommandAllocator(m_Type, IID_PPV_ARGS(&d3d_command_allocator)), "Could not create command allocator");
			RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateCommandList(0, m_Type, d3d_command_allocator.Get(), nullptr, IID_PPV_ARGS(&d3d_command_list)), "Could not create command list");
		
			command_list = new CommandList(d3d_command_list, d3d_command_allocator);
		}
		else
		{
			command_list = m_AvailableCommandLists[0];
			m_AvailableCommandLists.erase(m_AvailableCommandLists.begin());

			command_list->Reset();
		}

		return command_list;
	}

	void DeviceQueue::UpdateAvailableCommandLists()
	{
		auto itr = m_RunningCommandLists.begin();
		while (itr != m_RunningCommandLists.end())
		{
			if (IsFenceReached(itr->fenceValue))
			{
				m_AvailableCommandLists.push_back(itr->commandList);
				itr = m_RunningCommandLists.erase(itr);
			}
			else
			{
				++itr;
			}
		}
	}

	uint64_t DeviceQueue::ExecuteCommandList(CommandList* command_list)
	{
		CommandList* ptr[] = { command_list };

		return ExecuteCommandLists(1, ptr);
	}

	uint64_t DeviceQueue::ExecuteCommandLists(uint32_t num_command_lists, CommandList** command_lists)
	{
		ID3D12CommandList** d3d_lists = (ID3D12CommandList**)alloca(sizeof(ID3D12CommandList*) * num_command_lists);

		// Close the command lists
		for (uint32_t i = 0; i < num_command_lists; ++i)
		{
			command_lists[i]->GetCommandList()->Close();

			d3d_lists[i] = command_lists[i]->GetCommandList();
		}

		// Execute command lists
		m_CommandQueue->ExecuteCommandLists(num_command_lists, d3d_lists);
		uint64_t fence_value = SignalFence();

		for (uint32_t i = 0; i < num_command_lists; ++i)
		{
			m_RunningCommandLists.push_back({ fence_value, command_lists[i] });
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

	void DeviceQueue::WaitForFenceValue(uint64_t fence_value, uint64_t max_duration_ms)
	{
		if (IsFenceReached(fence_value))
		{
			return;
		}
		
		RB_ASSERT_FATAL_RELEASE_D3D(m_Fence->SetEventOnCompletion(fence_value, m_FenceEventHandle), "Could not hang event on fence");
		::WaitForSingleObject(m_FenceEventHandle, static_cast<DWORD>(max_duration_ms));
	}
}