#include "../shared/Common.h"
#include "GBuffer.h"

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

struct PO_GBufferEncodedOutput
{
    float4 gbuf0 : SV_Target0;
    float4 gbuf1 : SV_Target1;
};

PI_SIMPLE VS_Gbuffer(VI_Simple input)
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

PO_GBufferEncodedOutput PS_Gbuffer(PI_SIMPLE input)
{
    GBuffer gbuf;

    bool tex_is_srgb;
    float4 color = FetchTex2D(1, tex_is_srgb).Sample(g_ClampAnisoSampler, input.uv);

    // Somehow input.position.z is always outputted as 0, maybe not enough precision?
    float linear_depth = ((input.position.z / input.position.w) + 1.0f) * 0.5f;
    float2 near_far = ExtractNearFar();

    gbuf.color  = float4(color.rgb, 1.0f);
    gbuf.normal = input.normal;
    gbuf.depth  = LinearizeDepth(linear_depth, near_far.x, near_far.y);

    if (tex_is_srgb)
    {
        // Convert from sRGB to linear space (apply inverted gamma)
        const float gamma = 2.2f; // Hardcoded gamma value, TODO is this always correct?
        gbuf.color.rgb = pow(gbuf.color.rgb, gamma);
    }

    GBufferEncoded enc = EncodeGBuffer(gbuf);

    PO_GBufferEncodedOutput output;
    output.gbuf0 = enc.gbuf0;
    output.gbuf1 = enc.gbuf1;

    return output;
}