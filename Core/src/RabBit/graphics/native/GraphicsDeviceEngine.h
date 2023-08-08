#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>

#include <queue>

namespace RB::Graphics::Native
{
	// D3D12 Command queue wrapper
	class GraphicsDeviceEngine
	{
	public:
		GraphicsDeviceEngine(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority, D3D12_COMMAND_QUEUE_FLAGS flags);
		~GraphicsDeviceEngine();

		uint64_t SignalFence();
		bool IsFenceReached(uint64_t fence_value);
		void WaitForFenceValue(uint64_t fence_value, uint64_t max_duration_ms = std::numeric_limits<uint64_t>::max());
		void WaitForIdle(uint64_t max_duration_ms = std::numeric_limits<uint64_t>::max());

		GPtr<ID3D12GraphicsCommandList2> GetCommandList();

		uint64_t ExecuteCommandList(GPtr<ID3D12GraphicsCommandList2> command_list);
		uint64_t ExecuteCommandLists(uint32_t num_command_lists, GPtr<ID3D12GraphicsCommandList2>* command_lists);

		GPtr<ID3D12CommandQueue> GetCommandQueue() const { return m_CommandQueue; }

	private:
		void CreateFence();

		struct CommandAllocatorEntry
		{
			uint64_t fenceValue;
			GPtr<ID3D12CommandAllocator> commandAllocator;
		};

		D3D12_COMMAND_LIST_TYPE							m_Type;
		GPtr<ID3D12CommandQueue>						m_CommandQueue;
		GPtr<ID3D12Fence>								m_Fence;
		uint64_t										m_FenceValue;
		HANDLE											m_FenceEventHandle;

		std::queue<CommandAllocatorEntry>				m_CommandAllocatorQueue;
		std::queue<GPtr<ID3D12GraphicsCommandList2>>	m_CommandListQueue;
	};
}