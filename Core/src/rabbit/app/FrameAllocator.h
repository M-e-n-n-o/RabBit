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

    using FrameAllocationPageSet = List<FrameAllocationPage*>;

    class FrameAllocator
    {
    public:
        FrameAllocator(const char* name, uint32_t frame_cycles, uint64_t page_size);
        ~FrameAllocator();

        void* Allocate(uint64_t size, uint64_t align = 1);

        template<typename T>
        T* Allocate(uint64_t amount);

        FrameAllocationPageSet LockCurrentPageSet();
        void UnlockPageSet(FrameAllocationPageSet set);

        void Cycle();

    private:
        Deque<FrameAllocationPage>   m_AllPages;
        Queue<FrameAllocationPage*>  m_FreePages;
        List<FrameAllocationPageSet> m_UsedPageSets;

        uint32_t                     m_CurrentPage;
        uint32_t                     m_FrameCycles;
        uint64_t                     m_PageSize;
        const char*                  m_Name;

        Mutex                        m_Mutex;
    };

    template<typename T>
    inline T* FrameAllocator::Allocate(uint64_t amount)
    {
        return (T*)Allocate(sizeof(T) * amount);
    }
}