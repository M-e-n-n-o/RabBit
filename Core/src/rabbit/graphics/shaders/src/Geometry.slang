module Geometry;
import Common;
import Transform;
import GBuffer;

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

ConstantBuffer<float4x4> g_LocalToWorldMat;

[shader("vertex")]
PI_SIMPLE Gbuffer(VI_Simple input)
{
    float3 world_pos = TransformLocalToWorld(input.position, g_LocalToWorldMat);
    float3 view_pos  = TransformWorldToView(world_pos);
    float4 clip_pos  = TransformViewToClip(view_pos);
        
    PI_SIMPLE output;
    output.position = clip_pos;
    output.normal   = normalize(mul((float3x3)g_LocalToWorldMat, input.normal));
    output.uv       = input.uv;

    return output;
}

struct PixelUniforms
{
    Texture2D<float4>.Handle albedo;
};

[shader("pixel")]
PO_GBufferEncodedOutput Gbuffer(PI_SIMPLE input, uniform PixelUniforms u)
{
    GBuffer gbuf;

    float4 color = u.albedo.Sample(g_WrapAnisoSampler, input.uv);

    gbuf.diffColor = float4(color.rgb, 1.0f);
    gbuf.normal    = input.normal;
    gbuf.depth     = LinearizeDepth(input.position.z);

    GBufferEncoded enc = EncodeGBuffer(gbuf);

    PO_GBufferEncodedOutput output;
    output.gbuf0 = enc.gbuf0;
    output.gbuf1 = enc.gbuf1;

    return output;
}