#include "RabBitCommon.h"
#include "FrameAllocator.h"

namespace RB
{
    FrameAllocator::FrameAllocator(const char* name, uint32_t frame_cycles, uint64_t page_size)
        : m_Name(name)
        , m_CurrentPage(0)
        , m_FrameCycles(frame_cycles)
        , m_PageSize(page_size)
    {
        RB_ASSERT(LOGTAG_MAIN, frame_cycles > 0, "Frame Cycles should be greater than 0");
        m_UsedPages.resize(frame_cycles);
    }

    FrameAllocator::~FrameAllocator()
    {
        m_AllPages.clear();
    }

    void* FrameAllocator::Allocate(uint64_t size, uint64_t align)
    {
        uint64_t final_size = Math::AlignUp(size, align);

        RB_ASSERT_FATAL_RELEASE(LOGTAG_MAIN, final_size <= m_PageSize, "Trying to allocate %d, which is more than the page size, increase the page size!", final_size);

        FrameAllocationPage* to_use = nullptr;
        for (FrameAllocationPage* page : m_UsedPages[m_CurrentPage].pages)
        {
            if (page->HasSpace(final_size))
            {
                to_use = page;
                break;
            }
        }

        if (to_use == nullptr)
        {
            if (m_FreePages.size() > 0)
            {
                // Get a free page
                to_use = m_FreePages.front();
                m_FreePages.pop();
            }
            else
            {
                RB_LOG(LOGTAG_MAIN, "Creating new frame page for: %s", m_Name);

                // If there are no free pages, create a new one
                m_AllPages.emplace_back(m_PageSize);
                to_use = &m_AllPages.back();
            }
            
            m_UsedPages[m_CurrentPage].pages.push_back(to_use);
        }

        return to_use->Allocate(final_size);
    }

    void FrameAllocator::Cycle()
    {
        m_CurrentPage = (m_CurrentPage + 1) % m_FrameCycles;

        // Free all the previously used pages of the new frame
        for (FrameAllocationPage* page : m_UsedPages[m_CurrentPage].pages)
        {
            page->Reset();
            m_FreePages.push(page);
        }
        m_UsedPages[m_CurrentPage].pages.clear();
    }

    FrameAllocationPage::FrameAllocationPage(uint64_t size)
        : m_Offset(0)
        , m_Size(size)
    {
        m_MemoryBlock = (uint8_t*)ALLOC_HEAP(size);
        RB_ASSERT_FATAL_RELEASE(LOGTAG_MAIN, m_MemoryBlock != nullptr, "Failed to allocate page memory");
    }

    FrameAllocationPage::~FrameAllocationPage()
    {
        SAFE_FREE(m_MemoryBlock);
    }

    void* FrameAllocationPage::Allocate(uint64_t size)
    {
        RB_ASSERT_FATAL_RELEASE(LOGTAG_MAIN, m_Offset + size <= m_Size, "Trying to allocate more space than the page has left");

        void* space = (void*)(m_MemoryBlock + m_Offset);
        m_Offset += size;

        return space;
    }

    bool FrameAllocationPage::HasSpace(uint64_t size)
    {
        return m_Offset + size <= m_Size;
    }

    void FrameAllocationPage::Reset()
    {
        m_Offset = 0;

#ifdef RB_CONFIG_DEBUG
        memset(m_MemoryBlock, 0, m_Size);
#endif
    }
}