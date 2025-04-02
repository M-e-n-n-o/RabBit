#include "../shared/Common.h"

cbuffer InstanceCB : CBUFFER_REG(kInstanceCB)
{
    float4x4 localToWorldMat;
}

struct VI_Simple
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
};

struct PI_SIMPLE
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
};

struct PO_SIMPLE
{
    float4 color    : SV_Target0;
    float4 normal   : SV_Target1;
};

PI_SIMPLE VS_Simple(VI_Simple input)
{
    float3 world_pos = TransformLocalToWorld(input.position, localToWorldMat);
    float3 view_pos  = TransformWorldToView(world_pos);
    float4 clip_pos  = TransformViewToClip(view_pos);

    PI_SIMPLE output;
    output.position = clip_pos;
    output.normal   = input.normal;
    output.uv       = input.uv;

    return output;
}

PO_SIMPLE PS_Simple(PI_SIMPLE input) : SV_TARGET
{
    PO_SIMPLE output;

    float4 color = FETCH_TEX2D(1).Sample(g_ClampAnisoSampler, input.uv);

    output.color    = float4(color.rgb, 1.0f);
    output.normal   = float4(input.normal, 0.0f);

    return output;
}