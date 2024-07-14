#pragma once

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	using DescriptorHandle = int32_t;

	class BindlessDescriptorHeap
	{
	public:
		BindlessDescriptorHeap(const wchar_t* name, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t max_descriptors);

		// Use this staging descriptor to create descriptors that will be inserted directly after creation into the BindlessDescriptorHeap
		D3D12_CPU_DESCRIPTOR_HANDLE GetStagingDestinationDescriptor() const { return m_CpuHeapStaged; }
		
		// Call this to copy over the newly created non shader-visible descriptor over to the shader visible heap
		DescriptorHandle InsertStagedDescriptor(DescriptorHandle* handle_overwrite = nullptr);
		
		void InvalidateDescriptor(DescriptorHandle handle);

		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuStart() const { return m_GpuStart; }
		GPtr<ID3D12DescriptorHeap> GetHeap() const { return m_GpuHeap; }
		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_Type; }

	private:
		List<bool>					m_AvailableSlots;
		uint32_t					m_NextAvailable;

		GPtr<ID3D12DescriptorHeap>	m_GpuHeap;
		GPtr<ID3D12DescriptorHeap>	m_CpuHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE	m_CpuHeapStaged;
		D3D12_GPU_DESCRIPTOR_HANDLE m_GpuStart;
		D3D12_DESCRIPTOR_HEAP_TYPE	m_Type;
		uint32_t					m_IncrementSize;
	};

	extern BindlessDescriptorHeap* g_BindlessTex2DHeap;
}