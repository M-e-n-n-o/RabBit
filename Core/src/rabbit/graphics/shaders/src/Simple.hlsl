#include "../shared/Common.h"

cbuffer InstanceCB : CBUFFER_REG(kInstanceCB)
{
    float4x4 projection;
}

// Simple color
// ---------------------------------------------------------------

struct VI_Simple
{
    float2 position : POSITION;
    float3 color    : TEXCOORD0;
};

struct PI_Simple
{
    float4 position : SV_POSITION;
    float3 color    : TEXCOORD0;
};

PI_Simple VS_Simple2D(VI_Simple input)
{
    PI_Simple output;
    output.position = mul(projection, float4(input.position, 1, 1));
    output.color    = input.color;

    return output;
}

float4 PS_Simple2D(PI_Simple input) : SV_Target0
{
    return float4(input.color, 1.0f);
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
    output.position = mul(projection, float4(input.position, 1, 1));
    output.uv = input.uv;

    return output;
}

float4 PS_SimpleTex2D(PI_SimpleTex input) : SV_Target0
{
    return FetchTex2D(0).Sample<float4>(g_ClampAnisoSampler, input.uv);
}