#ifndef RB_SHADER_CONSTANT_BUFFERS
#define RB_SHADER_CONSTANT_BUFFERS

#include "Common.h"

struct PresentCB
{
    float2 texOffset;
    float2 currSize;
    float  brightnessValue;
    float  gammaValue;
    float2 padding;
};
ALIGN_CHECK(PresentCB);

struct Light
{
    float3 worldPos;
    float3 color;
};

#endif