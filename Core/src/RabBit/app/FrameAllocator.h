#pragma once

namespace RB
{
    class FrameAllocationPage
    {
    public:
        FrameAllocationPage(uint64_t size);
        ~FrameAllocationPage();

        void* Allocate(uint64_t size);
        bool HasSpace(uint64_t size);
        void Reset();

    private:
        uint8_t* m_MemoryBlock;
        uint64_t m_Offset;
        uint64_t m_Size;
    };

    class FrameAllocator
    {
    public:
        FrameAllocator(const char* name, uint32_t frame_cycles, uint64_t page_size);
        ~FrameAllocator();

        void* Allocate(uint64_t size, uint64_t align = 1);

        template<typename T>
        T* Allocate(uint64_t amount);

        void Cycle();

    private:
        struct UsedPages
        {
            List<FrameAllocationPage*> pages;
        };

        Deque<FrameAllocationPage>   m_AllPages;
        Queue<FrameAllocationPage*>  m_FreePages;
        List<UsedPages>              m_UsedPages;

        uint32_t                     m_CurrentPage;
        uint32_t                     m_FrameCycles;
        uint64_t                     m_PageSize;
        const char*                  m_Name;
    };

    template<typename T>
    inline T* FrameAllocator::Allocate(uint64_t amount)
    {
        return (T*)Allocate(sizeof(T) * amount);
    }
}