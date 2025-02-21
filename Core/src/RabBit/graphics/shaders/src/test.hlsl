#include "../shared/Common.h"

cbuffer InstanceCB : CBUFFER_REG(kInstanceCB)
{
    float4x4 localToWorldMat;
}

struct vertexInfo
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
};

struct v2p
{
    float4 position : SV_POSITION;
    float3 normal    : NORMAL;
    float2 uv       : TEXCOORD0;
};

v2p VS_Simple(vertexInfo input)
{
    float3 world_pos = TransformLocalToWorld(input.position, localToWorldMat);
    float3 view_pos  = TransformWorldToView(world_pos);
    float4 clip_pos  = TransformViewToClip(view_pos);

    v2p output;
    output.position = clip_pos;
    output.normal   = input.normal;
    output.uv       = input.uv;

    return output;
}

float4 PS_Simple(v2p input) : SV_TARGET
{
    float4 color = FETCH_TEX2D(1).Sample(g_ClampAnisoSampler, input.uv);
    return float4(color.rgb, 1.0f);
}