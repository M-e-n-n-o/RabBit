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

        void RegisterDestructor(void* object, uint64_t amount, void (*destructor)(void*, uint64_t));

    private:
        struct DestructorEntry
        {
            void* object;
            uint64_t amount;
            void (*destructor)(void*, uint64_t);
        };

        List<DestructorEntry>  m_Destructors;
        uint8_t*               m_MemoryBlock;
        uint64_t               m_Offset;
        uint64_t               m_Size;
    };

    using FrameAllocationPageSet = List<FrameAllocationPage*>;

    class FrameAllocator
    {
    public:
        FrameAllocator(const char* name, uint32_t frame_cycles, uint64_t page_size);
        ~FrameAllocator();

        // !!! BE AWARE that constructors & destructors are not called on objects that are allocated from this method !!!
        void* Allocate(uint64_t size, uint64_t align = 1);

        template<typename T>
        T* Allocate(uint64_t amount = 1);

        FrameAllocationPageSet LockCurrentPageSet();
        void UnlockPageSet(FrameAllocationPageSet set);

        void Cycle();

    private:
        void* Allocate(FrameAllocationPage*& out_used_page, uint64_t size, uint64_t align = 1);

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
        RB_MUTEX_AUTO_LOCK(m_Mutex);

        FrameAllocationPage* used_page = nullptr;
        void* memory = Allocate(used_page, sizeof(T) * amount, alignof(T));

        T* objects = static_cast<T*>(memory);

        for (uint64_t i = 0; i < amount; i++)
        {
            new (&objects[i]) T();
        }

        used_page->RegisterDestructor(objects, amount, [](void* ptr, uint64_t count)
            {
                T* array = static_cast<T*>(ptr);
                for (uint64_t i = 0; i < count; i++)
                {
                    array[i].~T();
                }
            });

        return objects;
    }
}