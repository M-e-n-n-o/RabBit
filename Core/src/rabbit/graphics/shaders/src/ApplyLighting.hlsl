#include "../shared/Common.h"
#include "Transform.h"
#include "Lighting.h"
#include "GBuffer.h"

cbuffer ApplyLightingCB : CBUFFER_REG(kInstanceCB)
{
    ApplyLightingCB g_ApplyLighting;
}

float SampleShadowPCF(float2 uv, float depth, float bias)
{
    Tex2D shadow_map = FetchTex2D(2);

    float2 size = shadow_map.GetDimensions<float>();
    float2 texel_size = 1.0f / size;

    // 3x3 kernel around UV
    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float2 offset = float2(x, y) * texel_size;
            float sample_depth = shadow_map.Sample<float>(g_ClampPointSampler, uv + offset);

            // Standard depth test with bias
            sum += (sample_depth > (depth - bias)) ? 1.0f : 0.0f;
        }
    }

    return sum / 9.0f; // normalize to [0,1]
}

[numthreads(8, 8, 1)]
void CS_ApplyLightingDeferred(uint2 screen_coord : SV_DispatchThreadID)
{
    GBufferTexIndices indices;
    indices.gbuf0 = 0;
    indices.gbuf1 = 1;

    float2 uv = TransformPixelCoordsToScreenUVs(screen_coord);

    GBuffer gbuf = SampleGBuffer(indices, uv);

    if (gbuf.depth <= 0.00001f)
    {
        // Bail out, nothing to light up
        return;
    }

    float3 world_pos = TransformScreenUVsToWorld(uv, gbuf.depth, false);

    // shadow map coords
    float4 shadow_clip = mul(g_ApplyLighting.shadowVP, float4(world_pos, 1.0f));
    float3 shadow_ndc = shadow_clip.xyz / shadow_clip.w;

    // convert from NDC [-1,1] to UV [0,1]
    float2 shadow_uv = float2(shadow_ndc.x * 0.5f + 0.5f,
                              1.0f - (shadow_ndc.y * 0.5f + 0.5f));

    // Sample shadow map
    float shadow = SampleShadowPCF(shadow_uv, shadow_ndc.z, 0.0005f);

    // Calculate lighting
    float3 diffuse;
    float3 specular;
    float3 ambient = gbuf.color.rgb * 0.05f;
    GetBlinnPhongDiffSpec(GetCameraPos(),
                          world_pos,
                          gbuf.normal,
                          69.0f,
                          g_ApplyLighting.light,
                          diffuse,
                          specular);

    diffuse  *= shadow;
    specular *= shadow;

    float4 final_color = float4(ambient + diffuse + specular, 1.0f);
    FetchRWTex2D(0).Store<float4>(screen_coord, final_color);
}