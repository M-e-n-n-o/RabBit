#include "RabBitCommon.h"
#include "BindlessDescriptorHeap.h"
#include "graphics/d3d12/GraphicsDevice.h"

namespace RB::Graphics::D3D12
{
	BindlessDescriptorHeap::BindlessDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t max_descriptors)
		: m_Type(type)
		, m_NextAvailable(0)
	{
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, max_descriptors < 1e6, "The max total of descriptors cannot exceed 1 million");

		m_AvailableSlots.reserve(max_descriptors);
		for (uint32_t i = 0; i < max_descriptors; ++i)
		{
			m_AvailableSlots[i] = true;
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
		}

		// Non shader-visible descriptor heap
		{
			D3D12_DESCRIPTOR_HEAP_DESC cpu_desc = {};
			cpu_desc.Type			= m_Type;
			cpu_desc.NumDescriptors = 1;
			cpu_desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			cpu_desc.NodeMask		= 0;

			RB_ASSERT_FATAL_D3D(g_GraphicsDevice->Get()->CreateDescriptorHeap(&cpu_desc, IID_PPV_ARGS(&m_CpuHeap)),
				"Failed to create CPU visible descriptor heap");

			m_CpuHeapStaged = m_CpuHeap->GetCPUDescriptorHandleForHeapStart();
		}
	}

	DescriptorHandle BindlessDescriptorHeap::InsertStagedDescriptor()
	{
		uint32_t starting_point = m_NextAvailable;

		// Find the first available slot in the heap
		while (!m_AvailableSlots[m_NextAvailable])
		{
			m_NextAvailable = (m_NextAvailable + 1) % m_AvailableSlots.size();

			if (starting_point == m_NextAvailable)
			{
				RB_ASSERT_ALWAYS(LOGTAG_GRAPHICS, "Could not find a free slot in the descriptor heap, overwriting a used one. Consider increasing the heap size!");
				return -1;
			}
		}

		uint32_t slot = m_NextAvailable;

		m_AvailableSlots[slot] = false;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
		cpu_handle.InitOffsetted(m_GpuHeap->GetCPUDescriptorHandleForHeapStart(), slot, m_IncrementSize);

		g_GraphicsDevice->Get()->CopyDescriptorsSimple(1, cpu_handle, m_CpuHeapStaged, m_Type);

		m_NextAvailable = (m_NextAvailable + 1) % m_AvailableSlots.size();

		return slot;
	}

	void BindlessDescriptorHeap::InvalidateDescriptor(DescriptorHandle handle)
	{
		m_AvailableSlots[handle] = true;
	}
}