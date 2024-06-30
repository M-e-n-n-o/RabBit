#include "RabBitCommon.h"
#include "DescriptorAllocation.h"

namespace RB::Graphics::D3D12
{
	DescriptorAllocation::DescriptorAllocation()
		: m_Descriptor{0}
		, m_NumHandles(0)
		, m_DescriptorSize(0)
	{
	}

	DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t num_handles, uint32_t descriptor_size)
		: m_Descriptor(descriptor)
		, m_NumHandles(num_handles)
		, m_DescriptorSize(descriptor_size)
	{
	}

	DescriptorAllocation::~DescriptorAllocation()
	{
		Free();
	}

	DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation)
		: m_Descriptor(allocation.m_Descriptor)
		, m_NumHandles(allocation.m_NumHandles)
		, m_DescriptorSize(allocation.m_DescriptorSize)
	{
		allocation.m_Descriptor.ptr = 0;
		allocation.m_NumHandles		= 0;
		allocation.m_DescriptorSize = 0;
	}

	DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other)
	{
		Free();

		m_Descriptor	 = other.m_Descriptor;
		m_NumHandles	 = other.m_NumHandles;
		m_DescriptorSize = other.m_DescriptorSize;

		other.m_Descriptor.ptr = 0;
		other.m_NumHandles	   = 0;
		other.m_DescriptorSize = 0;

		return *this;
	}

	bool DescriptorAllocation::IsNull() const
	{
		return m_Descriptor.ptr == 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(uint32_t offset) const
	{
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, offset < m_NumHandles, "Offset is not valid");

		return { m_Descriptor.ptr + (m_DescriptorSize * offset) };
	}

	uint32_t DescriptorAllocation::GetNumHandles() const
	{
		return m_NumHandles;
	}

	void DescriptorAllocation::Free()
	{
		if (IsNull())
		{
			return;
		}

		m_Descriptor.ptr = 0;
		m_NumHandles	 = 0;
		m_DescriptorSize = 0;

		// TODO Notify the page that the descriptor is free'd and reset the page
		static_assert(false);
	}
}