#include "../shared/Common.h"

cbuffer InstanceCB : CBUFFER_REG(kInstanceCB)
{
    float4x4 projection;
}

struct VI_Font
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct PI_Font
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

PI_Font VS_Font2D(VI_Font input)
{
    PI_Font output;
    output.position = mul(projection, float4(input.position, 1, 1));
    output.uv = input.uv;

    return output;
}

float4 PS_Font2D(PI_Font input) : SV_Target0
{
    float alpha = FetchTex2D(0).Sample<float>(g_ClampAnisoSampler, input.uv);
    return float4(1.0, 1.0, 1.0, alpha);
}