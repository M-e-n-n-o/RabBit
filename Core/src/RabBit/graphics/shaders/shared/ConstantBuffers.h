#pragma once

#if !SHADER
#include "RabBitCommon.h"
#endif

#include "Common.h"


struct PresentCB
{
    float2 texOffset;
    float2 currSize;
    float  brightnessValue;
    float  gammaValue;
};