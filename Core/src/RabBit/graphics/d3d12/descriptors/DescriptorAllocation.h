#pragma once

#include "RabBitCommon.h"

#include <d3d12.h>
#include <d3dx12/d3dx12.h>

namespace RB::Graphics::D3D12
{
	class DescriptorAllocation
	{
	public:
		// NULL descriptor
		DescriptorAllocation();

		DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t num_handles, uint32_t descriptor_size);

		~DescriptorAllocation();

		// No copies
		DescriptorAllocation(const DescriptorAllocation&) = delete;
		DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

		// Move
		DescriptorAllocation(DescriptorAllocation&& allocation);
		DescriptorAllocation& operator=(DescriptorAllocation&& other);

		bool IsNull() const;

		D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

		uint32_t GetNumHandles() const;

	private:
		void Free();

		D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;
		uint32_t					m_NumHandles;
		uint32_t					m_DescriptorSize;
	};
}