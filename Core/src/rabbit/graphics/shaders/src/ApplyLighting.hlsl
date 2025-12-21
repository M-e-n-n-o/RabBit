#include "../shared/Common.h"
#include "Transform.h"
#include "Lighting.h"
#include "GBuffer.h"

cbuffer ApplyLightingCB : CBUFFER_REG(kInstanceCB)
{
    ApplyLightingCB g_ApplyLighting;
}

float SampleShadowPCF(float2 uv, float depth, float slice, float bias)
{
    Tex2DArray shadow_map = FetchTex2DArray(2);

    float width, height, elements;
    shadow_map.GetDimensions<float>(width, height, elements);
    float2 texel_size = 1.0f / float2(width, height);

    // 3x3 kernel around UV
    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float2 offset = float2(x, y) * texel_size;
            float sample_depth = shadow_map.Sample<float>(g_ClampAnisoSampler, float3(uv + offset, slice));

            // Standard depth test with bias
            sum += (sample_depth > (depth - bias)) ? 1.0f : 0.0f;
        }
    }

    return sum / 9.0f; // Normalize to [0,1]
}

[numthreads(8, 8, 1)]
void CS_ApplyLightingDeferred(uint2 screen_coord : SV_DispatchThreadID)
{
    GBufferTexIndices indices;
    indices.gbuf0 = 0;
    indices.gbuf1 = 1;

    GBuffer gbuf = SampleGBuffer(indices, screen_coord);

    float view_depth = gbuf.depth;

    if (view_depth <= 0.00001f)
    {
        // Bail out, nothing to light up
        return;
    }

    float2 uv = TransformPixelCoordsToScreenUVs(screen_coord);
    float3 world_pos = TransformScreenUVsToWorld(uv, view_depth, false);

    // Find cascade
    uint cascade_idx = 0;
    for (int i = 0; i < min(g_ApplyLighting.cascades, MAX_NUM_CASCADES) - 1; i++)
    {
        if (view_depth > g_ApplyLighting.cascadeSplits[i])
            cascade_idx = i + 1;
    }

    // Shadow map coords
    float4 shadow_clip = mul(g_ApplyLighting.shadowVPs[cascade_idx], float4(world_pos, 1.0f));
    float3 shadow_ndc = shadow_clip.xyz / shadow_clip.w;

    // Convert from NDC [-1,1] to UV [0,1]
    float2 shadow_uv = float2(shadow_ndc.x * 0.5f + 0.5f,
                              1.0f - (shadow_ndc.y * 0.5f + 0.5f));

    // Sample shadow map
    float shadow = SampleShadowPCF(shadow_uv, shadow_ndc.z, cascade_idx, 0.0001f);

    // Calculate lighting
    float3 diffuse;
    float3 specular;
    float3 ambient = gbuf.diffColor.rgb * 0.01f;
    GetBlinnPhongBRDF(GetCameraPos(),
                      world_pos,
                      gbuf.normal,
                      69.0f,
                      g_ApplyLighting.light,
                      diffuse,
                      specular);

    diffuse  *= shadow;
    specular *= shadow;

    float4 final_color = float4(ambient + diffuse + specular, 1.0f);
    FetchRWTex2D(3).Store<float4>(screen_coord, final_color);
}