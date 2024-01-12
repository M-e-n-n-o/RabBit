#pragma once

#include "RabBitCommon.h"
#include "resource/Buffer.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	// Helper class to wrap command list calls
	class CommandList
	{
	public:
		CommandList(GPtr<ID3D12GraphicsCommandList2> command_list, GPtr<ID3D12CommandAllocator> command_allocator);
		~CommandList();

		void TransitionResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES to, uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		void FlushResourceBarriers();

		void UploadBuffer(Buffer* buffer);

		// Used to track objects until this command list has been finished executing (when Reset() is called)
		void TrackObjectUntilExecuted(GPtr<ID3D12Object> object);

		void Reset();

		ID3D12GraphicsCommandList2* GetCommandList()		const { return m_CommandList.Get(); }
		ID3D12CommandAllocator*		GetCommandAllocator()	const { return m_CommandAllocator.Get(); }

	private:
		GPtr<ID3D12GraphicsCommandList2>	m_CommandList;
		GPtr<ID3D12CommandAllocator>		m_CommandAllocator;

		// Will be released when the command list is reset
		List<GPtr<ID3D12Object>>			m_TrackedObjects;
	};
}