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

struct LightCB
{
    float3 direction;
    float  pad0;
    float3 color;
    float  pad1;
};
ALIGN_CHECK(LightCB);

#endif