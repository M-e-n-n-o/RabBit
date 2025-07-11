#pragma once

namespace RB::Math
{
    #define kPI 3.14159265359
    
    #define ALIGN_8(x) Math::AlignUp((x), 8)

    template<typename T>
    inline T Abs(T value)
    {
        return value > 0 ? value : -value;
    }

    template<typename T>
    inline T Max(T a, T b)
    {
        return a > b ? a : b;
    }

    template<typename T>
    inline T Min(T a, T b)
    {
        return a < b ? a : b;
    }

    template<typename T>
    inline T Clamp(T value, T min, T max)
    {
        return Min(Max(value, min), max);
    }

    template<typename T>
    inline T Floor(T value)
    {
        return floor(value);
    }

    template<>
    inline float Floor<float>(float value)
    {
        return floorf(value);
    }

    template<typename T>
    inline T AlignUpWithMask(T value, size_t mask)
    {
        return (T)(((size_t)value + mask) & ~mask);
    }

    template<typename T>
    inline T AlignDownWithMask(T value, size_t mask)
    {
        return (T)((size_t)value & ~mask);
    }

    template<typename T>
    inline T AlignUp(T value, size_t alignment)
    {
        return AlignUpWithMask(value, alignment - 1);
    }

    template<typename T>
    inline T AlignDown(T value, size_t alignment)
    {
        return AlignDownWithMask(value, alignment - 1);
    }

    template<typename T>
    inline bool IsAligned(T value, size_t alignment)
    {
        return 0 == ((size_t)value & (alignment - 1));
    }

    template<typename T>
    inline T Sin(T value)
    {
        return sin(value);
    }

    template<>
    inline float Sin<float>(float value)
    {
        return sinf(value);
    }

    template<typename T>
    inline T Cos(T value)
    {
        return cos(value);
    }

    template<>
    inline float Cos<float>(float value)
    {
        return cosf(value);
    }

    template<typename T>
    inline T Tan(T value)
    {
        return tan(value);
    }

    template<>
    inline float Tan<float>(float value)
    {
        return tanf(value);
    }

    template<typename T>
    inline T ArcTan2(T y, T x)
    {
        return atan2(y, x);
    }

    template<>
    inline float ArcTan2<float>(float y, float x)
    {
        return atan2f(y, x);
    }

    template<typename T>
    inline T DegreesToRadians(T value)
    {
        return value * (kPI / 180.0f);
    }

    template<typename T>
    inline T RadiansToDegrees(T value)
    {
        return value * (180.0f / kPI);
    }
}