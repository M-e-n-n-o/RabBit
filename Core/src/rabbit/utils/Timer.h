#pragma once

#include "RabBitCommon.h"
#include <chrono>

namespace RB
{
    class Timer
    {
    public:
        using Clock = std::chrono::high_resolution_clock;

        Timer()
        {
            Reset();
        }

        void Reset()
        {
            m_Start = Clock::now();
        }

        double ElapsedSeconds()
        {
            auto now = Clock::now();
            std::chrono::duration<double> delta = now - m_Start;
            return delta.count();
        }

        double ElapsedMilliseconds()
        {
            auto now = Clock::now();
            std::chrono::duration<double, std::milli> delta = now - m_Start;
            return delta.count();
        }

    private:
        Clock::time_point m_Start;
    };
}