#pragma once

namespace RB
{
    // Custom data containers

    template<class T>
    using List = std::vector<T>;

    template<class T>
    using InitList = std::initializer_list<T>;

    template<class T, size_t Size>
    using Array = std::array<T, Size>;

    template<class T1, class T2>
    using Map = std::map<T1, T2>;

    template<class T1, class T2>
    using UnorderedMap = std::unordered_map<T1, T2>;

    template<class T>
    using Queue = std::queue<T>;
}