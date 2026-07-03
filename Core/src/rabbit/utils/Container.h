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

    template<class T0, class T1>
    using Map = std::map<T0, T1>;

    template<class T0, class T1>
    using UnorderedMap = std::unordered_map<T0, T1>;

    template<class T>
    using Queue = std::queue<T>;

    template<class T>
    using Deque = std::deque<T>;

    template<class T>
    using Stack = std::stack<T>;

    template<class T, class Hash = std::hash<T>>
    using UnorderedSet = std::unordered_set<T, Hash>;

    template<class T0, class T1>
    using Pair = std::pair<T0, T1>;
    template<typename T0, typename T1>
    constexpr auto MakePair(T0&& t0, T1&& t1)
    {
        using P0 = std::decay_t<T0>;
        using P1 = std::decay_t<T1>;
        return std::pair<P0, P1>(std::forward<T0>(t0), std::forward<T1>(t1));
    }
}