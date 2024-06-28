#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class DynamicDescriptorHeap
	{
	public:
		DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptors_per_heap);
		~DynamicDescriptorHeap();

		// Stages a range of CPU visible descriptors.
		// These are copied to a GPU visible descriptor heap
		// until the CommitStagedDescriptors function is called.
		void StageDescriptors(uint32_t root_parameter_index, uint32_t offset, uint32_t num_descriptors, const D3D12_CPU_DESCRIPTOR_HANDLE src_descriptors);
	};
}