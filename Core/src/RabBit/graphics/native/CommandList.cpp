#include "RabBitPch.h"
#include "CommandList.h"
#include "GraphicsDevice.h"

namespace RB::Graphics::Native
{
	CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type, uint32_t allocator_count, uint32_t first_allocator_index)
	{
		m_NativeCommandAllocators = new GPtr<ID3D12CommandAllocator>[allocator_count];
		for (uint32_t allocator_index = 0; allocator_index < allocator_count; ++allocator_index)
		{
			RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get2()->CreateCommandAllocator(type, IID_PPV_ARGS(&m_NativeCommandAllocators[allocator_index])),
				"Could not create command allocator");
		}

		RB_ASSERT_FATAL_RELEASE_D3D(g_GraphicsDevice->Get2()->CreateCommandList(0, type, m_NativeCommandAllocators[first_allocator_index].Get(), 
			nullptr, IID_PPV_ARGS(&m_NativeCommandList)), "Could not create command list");

		RB_ASSERT_FATAL_RELEASE_D3D(m_NativeCommandList->Close(), "Could not close command list");
	}

	CommandList::~CommandList()
	{
		delete[] m_NativeCommandAllocators;
	}

	void CommandList::Reset(uint32_t allocator_index)
	{
		m_NativeCommandAllocators[allocator_index]->Reset();
		m_NativeCommandList->Reset(m_NativeCommandAllocators[allocator_index].Get(), nullptr);
	}
}