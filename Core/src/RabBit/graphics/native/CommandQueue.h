#pragma once

#include "utils/Ptr.h"

// DirectX 12 specific headers.
#include <d3d12.h>

// D3D12 extension library.
#include <D3DX12/d3dx12.h>

#if defined(max)
#undef max
#endif

namespace RB::Graphics::Native
{
	class CommandQueue
	{
	public:
		CommandQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority, D3D12_COMMAND_QUEUE_FLAGS flags);
		~CommandQueue();

		uint64_t SignalFence();
		void WaitForFenceValue(uint64_t fence_value, uint64_t max_duration_ms = std::numeric_limits<uint64_t>::max());
		void Flush(uint64_t max_duration_ms = std::numeric_limits<uint64_t>::max());

		GPtr<ID3D12CommandQueue> Get() const { return m_NativeCommandQueue; }

	private:
		void CreateFence();

		GPtr<ID3D12CommandQueue>	m_NativeCommandQueue;
		GPtr<ID3D12Fence>			m_Fence;
		uint64_t					m_FenceValue;
		HANDLE						m_FenceEventHandle;
	};
}