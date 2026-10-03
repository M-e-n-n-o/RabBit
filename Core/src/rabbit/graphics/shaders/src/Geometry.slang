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
    float3 worldPos : TEXCOORD1;
};

struct PO_GBufferEncodedOutput
{
    float4 gbuf0 : SV_Target0;
    float4 gbuf1 : SV_Target1;
};

ConstantBuffer<GBufferCB> g_GBufferCB;

[shader("vertex")]
PI_SIMPLE Gbuffer(VI_Simple input)
{
    float3 world_pos = TransformLocalToWorld(input.position, g_GBufferCB.localToWorldMat);
    float3 view_pos  = TransformWorldToView(world_pos);
    float4 clip_pos  = TransformViewToClip(view_pos);
        
    PI_SIMPLE output;
    output.position = clip_pos;
    output.normal   = normalize(mul((float3x3)g_GBufferCB.localToWorldMat, input.normal));
    output.uv       = input.uv;
    output.worldPos = world_pos;

    return output;
}

struct PixelUniforms
{
    Texture2D<float4>.Handle albedo;
    Texture2D<float3>.Handle normal;
};

[shader("pixel")]
PO_GBufferEncodedOutput Gbuffer(PI_SIMPLE input, uniform PixelUniforms u)
{
    GBuffer gbuf;

    float4 color = u.albedo.Sample(g_WrapAnisoSampler, input.uv);

    float3 normal = input.normal;
    if (g_GBufferCB.hasNormalMap != 0)
    {
        float3 posDx = ddx(input.worldPos);
        float3 posDy = ddy(input.worldPos);
        float2 uvDx = ddx(input.uv);
        float2 uvDy = ddy(input.uv);

        float f = 1.0 / (uvDx.x * uvDy.y - uvDy.x * uvDx.y);
        float3 tangent = f * (uvDy.y * posDx - uvDx.y * posDy);
        tangent = normalize(tangent - normal * dot(normal, tangent)); // Orthogonalize
        float3 bitangent = cross(normal, tangent);

        float3 normal_map = u.normal.Sample(g_WrapAnisoSampler, input.uv) * 2.0f - 1.0f;
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        normal = normalize(mul(TBN, normal_map));
    }

    gbuf.diffColor = float4(color.rgb, 1.0f);
    gbuf.normal    = normal;
    gbuf.depth     = LinearizeDepth(input.position.z);

    GBufferEncoded enc = EncodeGBuffer(gbuf);

    PO_GBufferEncodedOutput output;
    output.gbuf0 = enc.gbuf0;
    output.gbuf1 = enc.gbuf1;

    return output;
}