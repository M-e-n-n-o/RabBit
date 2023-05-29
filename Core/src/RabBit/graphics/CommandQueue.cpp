#include "RabBitPch.h"
#include "CommandQueue.h"
#include "GraphicsDevice.h"

namespace RB::Graphics
{
	CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority, D3D12_COMMAND_QUEUE_FLAGS flags)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = type;
		desc.Priority = priority;
		desc.Flags = flags;
		desc.NodeMask = 0;

		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get2()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_NativeCommandQueue)), "Could not create command queue");

		CreateFence();
	}

	CommandQueue::~CommandQueue()
	{
		CloseHandle(m_FenceEventHandle);
	}

	void CommandQueue::Flush(uint64_t max_duration_ms)
	{
		uint64_t fence_value_for_signal = SignalFence();
		WaitForFenceValue(fence_value_for_signal, max_duration_ms);
	}
	
	void CommandQueue::CreateFence()
	{
		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)), "Could not create fence");

		m_FenceEventHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		RB_ASSERT_FATAL_RELEASE(m_FenceEventHandle, "Could not create fence event");

		m_FenceValue = 0;
	}
	
	uint64_t CommandQueue::SignalFence()
	{
		uint64_t fence_value_for_signal = ++m_FenceValue;
		RB_ASSERT_FATAL_RELEASE_D3D(m_NativeCommandQueue->Signal(m_Fence.Get(), fence_value_for_signal), "Could not signal the command queue");
		return fence_value_for_signal;
	}

	void CommandQueue::WaitForFenceValue(uint64_t fence_value, uint64_t max_duration_ms)
	{
		if (m_Fence->GetCompletedValue() >= fence_value)
		{
			return;
		}

		RB_ASSERT_FATAL_RELEASE_D3D(m_Fence->SetEventOnCompletion(fence_value, m_FenceEventHandle), "Could not hang event on fence");
		::WaitForSingleObject(m_FenceEventHandle, static_cast<DWORD>(max_duration_ms));
	}
}