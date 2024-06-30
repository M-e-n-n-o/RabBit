#include "RabBitCommon.h"
#include "DynamicGpuDescriptorHeap.h"
#include "graphics/d3d12/GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
	DynamicGpuDescriptorHeap::DynamicGpuDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptors_per_heap, std::function<void(D3D12_DESCRIPTOR_HEAP_TYPE, ID3D12DescriptorHeap*)> bind_descriptor_heap_callback)
		: m_HeapType(type)
		, m_NumDescriptorsPerHeap(descriptors_per_heap)
		, m_StaleDescriptorTableBitMask(0)
		, m_DescriptorTableBitMask(0)
		, m_CurrentCpuDescriptorHandle(D3D12_DEFAULT)
		, m_CurrentGpuDescriptorHandle(D3D12_DEFAULT)
		, m_NumFreeHandles(0)
		, m_BindDescriptorHeapCallback(bind_descriptor_heap_callback)
	{
		g_GraphicsDevice->Get()->GetDescriptorHandleIncrementSize(m_HeapType);

		// Allocate space for staging CPU visible descriptors
		m_DescriptorHandleCache = CreateUnique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_NumDescriptorsPerHeap);
	}

	DynamicGpuDescriptorHeap::~DynamicGpuDescriptorHeap()
	{
	}

	void DynamicGpuDescriptorHeap::StageDescriptors(uint32_t root_parameter_index, uint32_t offset, uint32_t num_descriptors, const D3D12_CPU_DESCRIPTOR_HANDLE src_descriptors)
	{
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, num_descriptors <= m_NumDescriptorsPerHeap, "Cannot stage more than the max number of descriptors per heap");
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, root_parameter_index <= c_MaxDescriptorTables, "Cannot stage more than c_MaxDescriptorTables root parameters");

		DescriptorTableCache& cache = m_DescriptorTableCache[root_parameter_index];

		// Check that the number of descriptors to copy do not exceed the number
		// of descriptors expected in the descriptor table.
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, (offset + num_descriptors) <= cache.numDescriptors, "Number of descriptors exceeds the number of descriptors in the descriptor table");

		D3D12_CPU_DESCRIPTOR_HANDLE* dst_desc = cache.baseDescriptor + offset;

		for (uint32_t i = 0; i < num_descriptors; ++i)
		{
			dst_desc[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(src_descriptors, i, m_DescriptorHandleIncrementSize);
		}

		// Set the root parameter index bit to make sure the descriptor table will be correctly bound
		m_StaleDescriptorTableBitMask |= (1 << root_parameter_index);
	}

	void DynamicGpuDescriptorHeap::CommitStagedDescriptorsForDraw(ID3D12GraphicsCommandList* command_list)
	{
		CommitStagedDescriptors(command_list, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	}

	void DynamicGpuDescriptorHeap::CommitStagedDescriptorsForDispatch(ID3D12GraphicsCommandList* command_list)
	{
		CommitStagedDescriptors(command_list, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DynamicGpuDescriptorHeap::CopyDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc)
	{
		if (!m_CurrentDescriptorHeap || m_NumFreeHandles < 1)
		{
			SetNewDescriptorHeap();
		}

		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = m_CurrentGpuDescriptorHandle;
		g_GraphicsDevice->Get()->CopyDescriptorsSimple(1, m_CurrentCpuDescriptorHandle, cpu_desc, m_HeapType);

		m_CurrentCpuDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
		m_CurrentGpuDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
		m_NumFreeHandles--;

		return gpu_handle;
	}

	void DynamicGpuDescriptorHeap::OnSetNewRootSignature(const RootSignatureParameterDescriptorDesc& descriptors_description)
	{
		// When the root signature changes, all descriptors have to be (re)bound
		m_StaleDescriptorTableBitMask = 0;

		m_DescriptorTableBitMask = descriptors_description.descriptorTableBitMask;

		uint32_t descriptor_table_bit_mask = m_DescriptorTableBitMask;

		uint32_t current_offset = 0;
		DWORD root_index;

		// Scan the set bits from least significant to most significant
		while (_BitScanForward(&root_index, descriptor_table_bit_mask) && root_index < descriptors_description.rootIndexSlotsUsed)
		{
			uint32_t num_descriptors = descriptors_description.numDescriptorsPerTable[root_index];

			DescriptorTableCache& cache = m_DescriptorTableCache[root_index];
			cache.numDescriptors = num_descriptors;
			cache.baseDescriptor = m_DescriptorHandleCache.get() + current_offset;

			current_offset += num_descriptors;

			// Flip the bit so it's not scanned again
			descriptor_table_bit_mask ^= (1 << root_index);
		}

		RB_ASSERT(LOGTAG_GRAPHICS, current_offset <= m_NumDescriptorsPerHeap, 
			"The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.");
	}

	void DynamicGpuDescriptorHeap::Reset()
	{
		m_CurrentDescriptorHeap.Reset();
		m_AvailableDescriptorHeaps		= m_DescriptorHeapPool;
		m_CurrentCpuDescriptorHandle	= CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		m_CurrentGpuDescriptorHandle	= CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		m_NumFreeHandles				= 0;
		m_DescriptorTableBitMask		= 0;
		m_StaleDescriptorTableBitMask	= 0;

		for (int i = 0; i < c_MaxDescriptorTables; ++i)
		{
			m_DescriptorTableCache[i].Reset();
		}
	}

	void DynamicGpuDescriptorHeap::CommitStagedDescriptors(ID3D12GraphicsCommandList* command_list, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> set_func)
	{
		uint32_t num_desc_to_commit = ComputeStaleDescriptorCount();

		if (num_desc_to_commit == 0)
		{
			return;
		}

		if (!m_CurrentDescriptorHeap || m_NumFreeHandles < num_desc_to_commit)
		{
			SetNewDescriptorHeap();
		}

		DWORD root_index;

		// Scan the set bits from least significant to most significant
		while (_BitScanForward(&root_index, m_StaleDescriptorTableBitMask))
		{
			UINT num_src_desc = m_DescriptorTableCache[root_index].numDescriptors;
			D3D12_CPU_DESCRIPTOR_HANDLE* src_desc_handles = m_DescriptorTableCache[root_index].baseDescriptor;

			D3D12_CPU_DESCRIPTOR_HANDLE dest_desc_range[] = { m_CurrentCpuDescriptorHandle };
			UINT des_desc_range_sizes[] = { num_src_desc };

			// Copy the staged CPU visible descriptor to the GPU visible descriptor heap
			g_GraphicsDevice->Get()->CopyDescriptors(1, dest_desc_range, des_desc_range_sizes, num_src_desc, src_desc_handles, nullptr, m_HeapType);

			// Set the descriptors on the command list
			set_func(command_list, root_index, m_CurrentGpuDescriptorHandle);

			m_CurrentCpuDescriptorHandle.Offset(num_src_desc, m_DescriptorHandleIncrementSize);
			m_CurrentGpuDescriptorHandle.Offset(num_src_desc, m_DescriptorHandleIncrementSize);
			m_NumFreeHandles -= num_src_desc;

			// Flip the stale bit so the descriptor is not recopied again unless updated with a new descriptor
			m_StaleDescriptorTableBitMask ^= (1 << root_index);
		}
	}

	void DynamicGpuDescriptorHeap::SetNewDescriptorHeap()
	{
		m_CurrentDescriptorHeap		 = RequestDescriptorHeap();
		m_NumFreeHandles			 = m_NumDescriptorsPerHeap;
		m_CurrentCpuDescriptorHandle = m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_CurrentGpuDescriptorHandle = m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		// Binds the current descriptor heap to the command list
		m_BindDescriptorHeapCallback(m_HeapType, m_CurrentDescriptorHeap.Get());

		// When updating the descriptor heap all descriptor tabled must be recopied to the new descriptor heap
		m_StaleDescriptorTableBitMask = m_DescriptorTableBitMask;
	}

	GPtr<ID3D12DescriptorHeap> DynamicGpuDescriptorHeap::RequestDescriptorHeap()
	{
		GPtr<ID3D12DescriptorHeap> descriptor_heap;

		if (!m_AvailableDescriptorHeaps.empty())
		{
			descriptor_heap = m_AvailableDescriptorHeaps.front();
			m_AvailableDescriptorHeaps.pop();
		}
		else
		{
			descriptor_heap = CreateDescriptorHeap();
			m_DescriptorHeapPool.push(descriptor_heap);
		}

		return descriptor_heap;
	}

	GPtr<ID3D12DescriptorHeap> DynamicGpuDescriptorHeap::CreateDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
		descriptor_heap_desc.Type			= m_HeapType;
		descriptor_heap_desc.NumDescriptors = m_NumDescriptorsPerHeap;
		descriptor_heap_desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		GPtr<ID3D12DescriptorHeap> descriptor_heap;
		RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap)), 
			"Failed to create GPU visible descriptor heap");

		return descriptor_heap;
	}

	uint32_t DynamicGpuDescriptorHeap::ComputeStaleDescriptorCount() const
	{
		DWORD i;
		uint32_t num_stale_descriptors = 0;
		DWORD stale_descriptors_mask = m_StaleDescriptorTableBitMask;

		// Scan the set bits from least significant to most significant
		while (_BitScanForward(&i, stale_descriptors_mask))
		{
			num_stale_descriptors += m_DescriptorTableCache[i].numDescriptors;
			stale_descriptors_mask ^= (1 << i);
		}

		return num_stale_descriptors;
	}

}