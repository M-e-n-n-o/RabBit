#include "../shared/Common.h"

cbuffer InstanceCB : CBUFFER_REG(kInstanceCB)
{
    float4x4 projection;
}

// Simple color
// ---------------------------------------------------------------

struct VI_SIMPLE
{
    float2 position : POSITION;
    float3 color    : TEXCOORD0;
};

struct PI_SIMPLE
{
    float4 position : SV_POSITION;
    float3 color    : TEXCOORD0;
};

PI_SIMPLE VS_Simple2D(VI_SIMPLE input)
{
    PI_SIMPLE output;
    output.position = mul(projection, float4(input.position, 1, 1));
    output.color    = input.color;

    return output;
}

float4 PS_Simple2D(PI_SIMPLE input) : SV_Target0
{
    return float4(input.color, 1.0f);
}

// Simple texture
// ---------------------------------------------------------------

struct VI_SIMPLETEX
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct PI_SIMPLETEX
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

PI_SIMPLETEX VS_SimpleTex2D(VI_SIMPLETEX input)
{
    PI_SIMPLETEX output;
    output.position = mul(projection, float4(input.position, 1, 1));
    output.uv = input.uv;

    return output;
}

float4 PS_SimpleTex2D(PI_SIMPLETEX input) : SV_Target0
{
    //return float4(input.uv.y, 0, 0, 1);
    float alpha = FetchTex2D(0).Sample<float>(g_ClampAnisoSampler, input.uv);
    return float4(1.0, 1.0, 1.0, alpha);
}