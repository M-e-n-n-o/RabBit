#include "RabBitCommon.h"
#include "UploadAllocator.h"
#include "ResourceManager.h"

namespace RB::Graphics::D3D12
{
	// ----------------------------------------------------------------------------
	//									UploadPage
	// ----------------------------------------------------------------------------

	UploadPage::UploadPage(const char* name, uint64_t size)
		: m_Name(name)
		, m_ResourceSize(size)
		, m_UploadOffset(0)
	{
		m_UploadResource = new GpuResource();
		g_ResourceManager->ScheduleCreateUploadResource(m_UploadResource, m_Name, m_ResourceSize);

		m_UploadResource->GetResource()->Map(0, nullptr, (void**)&m_WriteAddress);
		m_GpuAddress = m_UploadResource->GetResource()->GetGPUVirtualAddress();
	}
	
	UploadPage::~UploadPage()
	{
		delete m_UploadResource;
	}

	void UploadPage::Reset()
	{
		m_UploadOffset = 0;
	}

	bool UploadPage::HasSpace(uint64_t size, uint64_t alignment)
	{
		return Math::AlignUp(Math::AlignUp(m_UploadOffset, alignment) + size, alignment) <= m_ResourceSize;
	}

	UploadAllocation UploadPage::Allocate(uint64_t size, uint64_t alignment)
	{
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, HasSpace(size, alignment), L"Not enough space to allocate %d bytes on upload resource: %s", size, m_Name);

		uint64_t aligned_size = Math::AlignUp(size, alignment);
		m_UploadOffset = Math::AlignUp(m_UploadOffset, alignment);

		UploadAllocation location = {};
		location.resource			= m_UploadResource;
		location.maxWriteSize		= aligned_size;
		location.offset				= m_UploadOffset;
		location.address			= m_GpuAddress + m_UploadOffset;
		location.cpuWriteAddress	= m_WriteAddress + m_UploadOffset;

		m_UploadOffset += aligned_size;

		return location;
	}

	// ----------------------------------------------------------------------------
	//								UploadAllocator
	// ----------------------------------------------------------------------------

	UploadAllocator::UploadAllocator(const char* name, uint64_t page_size)
		: m_Name(name)
		, m_PageSize(page_size)
		, m_CurrentPage(-1)
		, m_ResetCounter(0)
	{

	}
	
	UploadAllocator::~UploadAllocator()
	{
		for (int i = 0; i < m_Pages.size(); i++)
		{
			delete m_Pages[i];
		}

		m_Pages.clear();
	}

	void UploadAllocator::Reset()
	{
		for (UploadPage* page : m_Pages)
		{
			page->Reset();
		}

		m_ResetCounter++;
		m_LastHighestPage = Math::Max(m_LastHighestPage, m_CurrentPage);

		// Every x resets, check if there are any pages not used, if so, delete them
		if (m_ResetCounter == m_DeleteCheck)
		{
			for (int i = m_LastHighestPage + 1; i < m_Pages.size(); ++i)
			{
				delete m_Pages[i];
			}

			m_Pages.resize(Math::Max(m_LastHighestPage + 1, 0));

			m_ResetCounter = 0;
			m_LastHighestPage = 0;
		}
			
		m_CurrentPage = 0;
	}

	UploadAllocation UploadAllocator::Allocate(uint64_t size, uint64_t alignment)
	{
		RB_ASSERT_FATAL(LOGTAG_GRAPHICS, size <= m_PageSize, L"Upload resource: %s, is too small to allocate %d bytes, increase the page size", m_Name, size);

		if (m_CurrentPage == -1)
		{
			m_CurrentPage = AddNewPage();
		}

		if (!m_Pages[m_CurrentPage]->HasSpace(size, alignment))
		{
			// Check if other pages have space
			bool found_space = false;
			for (int i = 0; i < m_Pages.size(); ++i)
			{
				if (m_Pages[i]->HasSpace(size, alignment))
				{
					found_space = true;

					// Make sure to keep track of which pages were used
					m_LastHighestPage = Math::Max(m_LastHighestPage, m_CurrentPage);

					m_CurrentPage = i;
					break;
				}
			}

			if (!found_space)
			{
				m_CurrentPage = AddNewPage();
			}
		}

		return m_Pages[m_CurrentPage]->Allocate(size, alignment);
	}
	
	int32_t UploadAllocator::AddNewPage()
	{
		std::string name(m_Name);
		name += " Page ";
		name += std::to_string(m_Pages.size());

		m_Pages.push_back(new UploadPage(name.c_str(), m_PageSize));

		return m_Pages.size() - 1;
	}
}