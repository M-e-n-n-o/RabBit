#pragma once

#include <functional>

namespace RB
{
    #ifndef _countof
        #define _countof(arr) (sizeof(arr) / sizeof(arr[0]))
    #endif

    template <class T>
    inline void HashCombine(uint64_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    // Convert a type to a constant ID that is the same on every machine (if compiled using the same compiler)
    template <typename T>
    consteval uint64_t ConstantTypeId()
    {
#if defined(__clang__) || defined(__GNUC__)
        const char* name = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
        const char* name = __FUNCSIG__;
#endif
        uint64_t h = 0;
        for (size_t i = 0; name[i] != '\0'; ++i)
        {
            char c = name[i];
            h ^= static_cast<uint64_t>(c) + 0x9e3779b9 + (c << 6) + (c >> 2);
        }
        return h;
    }

    uint32_t NumberOfSetBits(uint32_t bitset);
}