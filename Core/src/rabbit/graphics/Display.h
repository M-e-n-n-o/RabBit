#pragma once

#include "utils/Container.h"
#include "math/Vector.h"

namespace RB::Graphics
{
    class Display
    {
    public:
        virtual ~Display();

        virtual const char* GetName() = 0;

        virtual Math::Float2 GetResolution() = 0;

        virtual void* GetNativeHandle() = 0;

        static List<Display*> CreateDisplays();

    protected:
        Display();
    };
}