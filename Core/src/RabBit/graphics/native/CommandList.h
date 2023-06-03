#pragma once

#include "utils/Ptr.h"

// DirectX 12 specific headers.
#include <d3d12.h>

// D3D12 extension library.
#include <D3DX12/d3dx12.h>

namespace RB::Graphics::Native
{
	class CommandList
	{
	public:
		CommandList(D3D12_COMMAND_LIST_TYPE type, uint32_t allocator_count = 1, uint32_t first_allocator_index = 0);
		~CommandList();

		// Call this Reset instead of ID3D12GraphicsCommandList::Reset to properly reset the command list
		void Reset(uint32_t allocator_index = 0);

		GPtr<ID3D12GraphicsCommandList>	Get() const { return m_NativeCommandList; }

	private:
		GPtr<ID3D12GraphicsCommandList>	m_NativeCommandList;
		GPtr<ID3D12CommandAllocator>*	m_NativeCommandAllocators;
	};
}