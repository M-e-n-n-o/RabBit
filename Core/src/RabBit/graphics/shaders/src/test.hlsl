#include "../shared/Common.h"

cbuffer InstanceCB : CBUFFER_REG(kInstanceCB)
{
    float4x4 localToWorldMat;
}

struct vertexInfo
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct v2p
{
    float4 position : SV_POSITION;
    float3 color    : TEXCOORD0;
};

v2p VS_VertexColor(vertexInfo input)
{
    float3 world_pos    = TransformLocalToWorld(input.position, localToWorldMat);
    float3 view_pos     = TransformWorldToView(world_pos);
    float4 clip_pos     = TransformViewToClip(view_pos);

    v2p output;
    output.position = clip_pos;
    output.color    = input.color;

    return output;
}

float4 PS_VertexColor(v2p input) : SV_TARGET
{
    float4 color = FETCH_TEX2D(0).Sample(g_ClampSampler, float2(0, 0));
    return float4(color.rgb, 1.0f);
}