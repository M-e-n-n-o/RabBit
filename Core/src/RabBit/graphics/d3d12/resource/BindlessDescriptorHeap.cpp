#include "RabBitCommon.h"
#include "BindlessDescriptorHeap.h"
#include "graphics/d3d12/GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
	BindlessDescriptorHeap* g_BindlessSrvUavHeap = nullptr;

	BindlessDescriptorHeap::BindlessDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t max_descriptors)
		: m_Type(type)
	{
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, max_descriptors < 1e6, "The max total of descriptors cannot exceed 1 million");

		m_AvailableSlots.reserve(max_descriptors);
		for (uint32_t i = 0; i < max_descriptors; ++i)
		{
			m_AvailableSlots.push_back(true);
		}

		m_IncrementSize = g_GraphicsDevice->Get()->GetDescriptorHandleIncrementSize(m_Type);

		// Shader-visible descriptor heap
		{
			D3D12_DESCRIPTOR_HEAP_DESC gpu_desc = {};
			gpu_desc.Type			= m_Type;
			gpu_desc.NumDescriptors = max_descriptors;
			gpu_desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			gpu_desc.NodeMask		= 0;

			RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateDescriptorHeap(&gpu_desc, IID_PPV_ARGS(&m_GpuHeap)),
				"Failed to create GPU visible descriptor heap");

			m_GpuHeap->SetName(L"Bindless GPU heap");

			m_GpuStart = m_GpuHeap->GetGPUDescriptorHandleForHeapStart();
		}

		// Non shader-visible descriptor heap
		{
			// TODO Batch the descriptor copying together by doing the copy just before a draw/dispatch.
			// However still make the CPU heap a fixed size (always lower or equal to the size of the GPU heap, maybe size of 25?)
			// and if you have to copy more than, say that 25, between a draw, just do the copy of the 25 directly and reset the heap.

			D3D12_DESCRIPTOR_HEAP_DESC cpu_desc = {};
			cpu_desc.Type			= m_Type;
			cpu_desc.NumDescriptors = 1;
			cpu_desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			cpu_desc.NodeMask		= 0;

			RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateDescriptorHeap(&cpu_desc, IID_PPV_ARGS(&m_CpuHeap)),
				"Failed to create CPU visible descriptor heap");

			m_CpuHeap->SetName(L"Bindless CPU heap");

			m_CpuHeapStaged = m_CpuHeap->GetCPUDescriptorHandleForHeapStart();
		}
	}

	DescriptorHandle BindlessDescriptorHeap::InsertStagedDescriptor(uint32_t offset, uint32_t max_descriptors_type, DescriptorHandle* handle_overwrite)
	{
		uint32_t slot;

		if (handle_overwrite)
		{
			slot = *handle_overwrite;
		}
		else
		{
			slot = offset;

			// Find the first available slot in the heap
			while (!m_AvailableSlots[slot])
			{
				slot = (slot + 1) % max_descriptors_type;

				if (slot == offset)
				{
					RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Could not find a free slot in the descriptor heap, increase the heap size!");
					return -1;
				}
			}
		}

		m_AvailableSlots[slot] = false;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
		cpu_handle.InitOffsetted(m_GpuHeap->GetCPUDescriptorHandleForHeapStart(), slot, m_IncrementSize);

		g_GraphicsDevice->Get()->CopyDescriptorsSimple(1, cpu_handle, m_CpuHeapStaged, m_Type);

		return slot;
	}

	void BindlessDescriptorHeap::InvalidateDescriptor(DescriptorHandle handle)
	{
		m_AvailableSlots[handle] = true;
	}
}