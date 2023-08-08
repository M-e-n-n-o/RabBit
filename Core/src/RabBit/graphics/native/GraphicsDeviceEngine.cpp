#include "RabBitCommon.h"
#include "GraphicsDeviceEngine.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::Native
{
	GraphicsDeviceEngine::GraphicsDeviceEngine(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority, D3D12_COMMAND_QUEUE_FLAGS flags)
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

	GraphicsDeviceEngine::~GraphicsDeviceEngine()
	{
		CloseHandle(m_FenceEventHandle);
	}

	void GraphicsDeviceEngine::WaitForIdle(uint64_t max_duration_ms)
	{
		uint64_t fence_value_for_signal = SignalFence();
		WaitForFenceValue(fence_value_for_signal, max_duration_ms);
	}

	GPtr<ID3D12GraphicsCommandList2> GraphicsDeviceEngine::GetCommandList()
	{
		GPtr<ID3D12CommandAllocator> command_allocator;
		GPtr<ID3D12GraphicsCommandList2> command_list;

		if (!m_CommandAllocatorQueue.empty() && IsFenceReached(m_CommandAllocatorQueue.front().fenceValue))
		{
			command_allocator = m_CommandAllocatorQueue.front().commandAllocator;
			m_CommandAllocatorQueue.pop();

			RB_ASSERT_FATAL_RELEASE_D3D(command_allocator->Reset(), "Could not reset command allocator");
		}
		else
		{
			RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateCommandAllocator(m_Type, IID_PPV_ARGS(&command_allocator)), "Could not create command allocator");
		}

		if (!m_CommandListQueue.empty())
		{
			command_list = m_CommandListQueue.front();
			m_CommandListQueue.pop();

			RB_ASSERT_FATAL_RELEASE_D3D(command_list->Reset(command_allocator.Get(), nullptr), "Could not reset command list");
		}
		else
		{
			RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateCommandList(0, m_Type, command_allocator.Get(), nullptr, IID_PPV_ARGS(&command_list)), "Could not create command list");
		}

		// Associate the command allocator with the command list so that it can be
		// retrieved when the command list is executed.
		RB_ASSERT_FATAL_RELEASE_D3D(command_list->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), command_allocator.Get()), "Could not bind command allocator to command list");

		return command_list;
	}

	uint64_t GraphicsDeviceEngine::ExecuteCommandList(GPtr<ID3D12GraphicsCommandList2> command_list)
	{
		GPtr<ID3D12GraphicsCommandList2> ptr[] = { command_list };

		return ExecuteCommandLists(1, ptr);
	}

	uint64_t GraphicsDeviceEngine::ExecuteCommandLists(uint32_t num_command_lists, GPtr<ID3D12GraphicsCommandList2>* command_lists)
	{
		ID3D12CommandAllocator** command_allocators = (ID3D12CommandAllocator**)alloca(sizeof(ID3D12CommandAllocator*) * num_command_lists);

		UINT data_size = sizeof(command_allocators[0]);

		for (uint32_t i = 0; i < num_command_lists; ++i)
		{
			command_lists[i]->Close();

			RB_ASSERT_FATAL_RELEASE_D3D(command_lists[i]->GetPrivateData(__uuidof(ID3D12CommandAllocator), &data_size, &command_allocators[i]),
				"Could not retrieve command allocator from command list");
		}

		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, num_command_lists <= 5, "Can not execute more than 5 command lists at the moment!");
		ID3D12CommandList* const final_command_lists[] = { command_lists[0].Get(), command_lists[1].Get(), command_lists[2].Get(), command_lists[3].Get(), command_lists[4].Get() };

		m_CommandQueue->ExecuteCommandLists(num_command_lists, final_command_lists);
		uint64_t fence_value = SignalFence();

		for (uint32_t i = 0; i < num_command_lists; ++i)
		{
			m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fence_value, command_allocators[i] });
			m_CommandListQueue.push(command_lists[i]);

			// The ownership of the command allocator has been transferred to the ComPtr
			// in the command allocator queue. It is safe to release the reference 
			// in this temporary COM pointer here.
			command_allocators[i]->Release();
		}

		return fence_value;
	}
	
	void GraphicsDeviceEngine::CreateFence()
	{
		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)), "Could not create fence");

		m_FenceEventHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, m_FenceEventHandle, "Could not create fence event");

		m_FenceValue = 0;
	}
	
	uint64_t GraphicsDeviceEngine::SignalFence()
	{
		uint64_t fence_value_for_signal = ++m_FenceValue;
		RB_ASSERT_FATAL_RELEASE_D3D(m_CommandQueue->Signal(m_Fence.Get(), fence_value_for_signal), "Could not signal the command queue");
		return fence_value_for_signal;
	}

	bool GraphicsDeviceEngine::IsFenceReached(uint64_t fence_value)
	{
		return m_Fence->GetCompletedValue() >= fence_value;
	}

	void GraphicsDeviceEngine::WaitForFenceValue(uint64_t fence_value, uint64_t max_duration_ms)
	{
		if (IsFenceReached(fence_value))
		{
			return;
		}
		
		RB_ASSERT_FATAL_RELEASE_D3D(m_Fence->SetEventOnCompletion(fence_value, m_FenceEventHandle), "Could not hang event on fence");
		::WaitForSingleObject(m_FenceEventHandle, static_cast<DWORD>(max_duration_ms));
	}
}