#pragma once

#include <memory>

namespace RB
{
    #define kKB(x) (x * 1024)
    #define kMB(x) (x * 1024 * 1024)
    
    #define k32KB   kKB(32)
    #define k64KB   kKB(64)
    #define k1MB    kMB(1)
    #define k2MB    kMB(2)
    #define k4MB    kMB(4)
    #define k8MB    kMB(8)
    #define k16MB   kMB(16)
    #define k32MB   kMB(32)
    #define k64MB   kMB(64)
    #define k128MB  kMB(128)
    #define k256MB  kMB(256)

    #define ALLOC_STACK(size)           alloca((size))
    #define ALLOC_STACKC(type, count)   (type*)alloca(sizeof(type) * (count))

    // Try to use the FrameAllocator when allocating & freeing memory from the heap every frame!
    #define ALLOC_HEAP(size)            malloc((size))
    #define ALLOC_HEAPC(type, count)    (type*)malloc(sizeof(type) * (count))
    
    #define SAFE_RELEASE(obj)           (obj)->Release();
    #define SAFE_DELETE(obj)            if ((obj) != nullptr) { delete (obj); (obj) = nullptr; }
    #define SAFE_DELETE_ARR(obj)        if ((obj) != nullptr) { delete[] (obj); (obj) = nullptr; }
    #define SAFE_FREE(obj)              if ((obj) != nullptr) { free(obj); (obj) = nullptr; }

    // Custom shared pointer
    template<typename T>
    using Shared = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Shared<T> CreateShared(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename T2>
    constexpr Shared<T> CastShared(Shared<T2> base)
    {
        return std::dynamic_pointer_cast<T>(base);
    }

    // Custom unique pointer
    template<typename T>
    using Unique = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Unique<T> CreateUnique(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
}