#pragma once

#include <functional>

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

    template <class T>
    inline void HashCombine(uint64_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    uint32_t NumberOfSetBits(uint32_t bitset);
}