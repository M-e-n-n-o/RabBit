#pragma once

#include "RabBitCommon.h"
#include "graphics/d3d12/Pipeline.h"

#include <d3d12.h>
#include <d3dx12/d3dx12.h>

namespace RB::Graphics::D3D12
{
	class DynamicGpuDescriptorHeap
	{
	public:
		DynamicGpuDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptors_per_heap, std::function<void()> bind_descriptor_heap_callback);
		~DynamicGpuDescriptorHeap();

		// Stages a range of CPU visible descriptors. These are copied to a GPU visible 
		// descriptor heap until the CommitStagedDescriptors function is called.
		void StageDescriptors(uint32_t root_parameter_index, uint32_t offset, uint32_t num_descriptors, const D3D12_CPU_DESCRIPTOR_HANDLE src_descriptors);

		// Copies all of the staged descriptors to the GPU visible descriptor heap and
		// binds the heap and descriptor tables to the command list.
		void CommitStagedDescriptorsForDraw(ID3D12GraphicsCommandList* command_list);
		void CommitStagedDescriptorsForDispatch(ID3D12GraphicsCommandList* command_list);

		// Copies a single CPU visible descriptor to a GPU visible descriptor heap.
		// Useful for ClearUAV calls, which require both CPU and GPU visible descriptors.
		D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc);

		// Determines which descriptors on which root signature slot have to be set
		void OnSetNewRootSignature(const RootSignatureParameterDescriptorDesc& descriptors_description);

		// Reset used descriptors.
		// Only call when the descriptors are finished executing on the GPU!
		void Reset();

		GPtr<ID3D12DescriptorHeap> GetCurrentDescriptorHeap() const { return m_CurrentDescriptorHeap; }

	private:
		void CommitStagedDescriptors(ID3D12GraphicsCommandList* command_list, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> set_func);
		
		void SetNewDescriptorHeap();
		GPtr<ID3D12DescriptorHeap> RequestDescriptorHeap();
		GPtr<ID3D12DescriptorHeap> CreateDescriptorHeap();

		uint32_t ComputeStaleDescriptorCount() const;

		struct DescriptorTableCache
		{
			DescriptorTableCache()
				: numDescriptors(0)
				, baseDescriptor(nullptr)
			{}

			void Reset()
			{
				numDescriptors = 0;
				baseDescriptor = nullptr;
			}

			uint32_t					 numDescriptors;
			D3D12_CPU_DESCRIPTOR_HANDLE* baseDescriptor;
		};

		// Max number of descriptor tables per root signature
		static const uint32_t					c_MaxDescriptorTables = 32;

		D3D12_DESCRIPTOR_HEAP_TYPE				m_HeapType;
		uint32_t								m_NumDescriptorsPerHeap;
		uint32_t								m_DescriptorHandleIncrementSize;

		Unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>	m_DescriptorHandleCache;
		DescriptorTableCache					m_DescriptorTableCache[c_MaxDescriptorTables];

		uint32_t								m_StaleDescriptorTableBitMask;
		uint32_t								m_DescriptorTableBitMask;

		Queue<GPtr<ID3D12DescriptorHeap>>		m_DescriptorHeapPool;
		Queue<GPtr<ID3D12DescriptorHeap>>		m_AvailableDescriptorHeaps;

		GPtr<ID3D12DescriptorHeap>				m_CurrentDescriptorHeap;
		CD3DX12_CPU_DESCRIPTOR_HANDLE			m_CurrentCpuDescriptorHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE			m_CurrentGpuDescriptorHandle;

		uint32_t								m_NumFreeHandles;

		std::function<void()>					m_BindDescriptorHeapCallback;
	};
}