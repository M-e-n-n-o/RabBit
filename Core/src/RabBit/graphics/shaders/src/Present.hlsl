#include "../shared/Common.h"

struct vertexInfo
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct v2p
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

v2p VS_Present(vertexInfo input)
{
    v2p output;
    output.position = float4(input.position, 0, 1);
    output.uv       = input.uv;

    return output;
}

float4 PS_Present(v2p input) : SV_TARGET
{
    return FETCH_TEX2D(0).Sample(g_ClampSampler, input.uv);
}