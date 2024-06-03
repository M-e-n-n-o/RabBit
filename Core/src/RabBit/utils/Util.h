#pragma once

#include <functional>

namespace RB
{
    template <class T>
    inline void HashCombine(uint64_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    uint32_t NumberOfSetBits(uint32_t bitset);
}