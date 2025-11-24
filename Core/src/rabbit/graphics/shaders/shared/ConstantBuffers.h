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
    float3 direction;
    float  pad0;
    float3 color;
    float  pad1;
};

#define MAX_NUM_CASCADES 4

struct ApplyLightingCB
{
    float4x4 shadowVPs[MAX_NUM_CASCADES];
    float4   cascadeSplits; // As float4 cause array's cause padding in HLSL

    uint32_t cascades;
    float3   padding;

    Light    light;
};
ALIGN_CHECK(ApplyLightingCB);

#endif