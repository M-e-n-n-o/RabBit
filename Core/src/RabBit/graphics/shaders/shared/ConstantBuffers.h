#ifndef RB_SHADER_CONSTANT_BUFFERS
#define RB_SHADER_CONSTANT_BUFFERS

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

#endif