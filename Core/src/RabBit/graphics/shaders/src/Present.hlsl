#include "../shared/Common.h"
#include "../shared/ConstantBuffers.h"

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

cbuffer PresentCB : CBUFFER_REG(kInstanceCB)
{
    PresentCB g_PresentCB;
}

v2p VS_Present(vertexInfo input)
{
    v2p output;
    output.position = float4(input.position, 0, 1);
    output.uv       = input.uv;

    return output;
}

float4 PS_Present(v2p input) : SV_TARGET
{
    float2 screen_coords = input.uv * g_PresentCB.currSize;
    float2 tex_coords    = screen_coords - g_PresentCB.texOffsetAndSize.xy;
    float2 tex_uv        = tex_coords / g_PresentCB.texOffsetAndSize.zw;

    return FETCH_TEX2D(0).Sample(g_BlackBorderSampler, tex_uv);
}