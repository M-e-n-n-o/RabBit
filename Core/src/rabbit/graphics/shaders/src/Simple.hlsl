#include "../shared/Common.h"
#include "Transform.h"

cbuffer InstanceCB : CBUFFER_REG(kInstanceCB)
{
    float4x4 customMatrix;
}

// Simple color
// ---------------------------------------------------------------

struct VI_Simple
{
    float2 position : POSITION;
    float4 color    : TEXCOORD0;
};

struct PI_SimpleCol
{
    float4 position : SV_POSITION;
    float4 color    : TEXCOORD0;
};

struct PI_Simple
{
    float4 position : SV_POSITION;
};

PI_SimpleCol VS_Simple2D(VI_Simple input)
{
    PI_SimpleCol output;
    output.position = mul(customMatrix, float4(input.position, 1, 1));
    output.color    = input.color;

    return output;
}

float4 PS_Simple2D(PI_SimpleCol input) : SV_Target0
{
    return input.color;
}

float4 PS_SimpleDepth(PI_Simple input) : SV_Target0
{
    return input.position.zzzz;
}

// Simple texture
// ---------------------------------------------------------------

struct VI_SimpleTex
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct PI_SimpleTex
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

PI_SimpleTex VS_SimpleTex2D(VI_SimpleTex input)
{
    PI_SimpleTex output;
    output.position = mul(customMatrix, float4(input.position, 1, 1));
    output.uv = input.uv;

    return output;
}

float4 PS_SimpleTex2D(PI_SimpleTex input) : SV_Target0
{
    return FetchTex2D(0).Sample<float4>(g_ClampAnisoSampler, input.uv);
}

// Simple 3D
// ---------------------------------------------------------------

struct VI_Simple3D
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
};

PI_Simple VS_Simple3D(VI_Simple3D input)
{
    float3 world_pos = TransformLocalToWorld(input.position, customMatrix);
    float3 view_pos  = TransformWorldToView(world_pos);
    float4 clip_pos  = TransformViewToClip(view_pos);

    PI_Simple output;
    output.position = clip_pos;
    return output;
}