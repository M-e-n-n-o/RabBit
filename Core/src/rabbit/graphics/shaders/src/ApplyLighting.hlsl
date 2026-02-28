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

    // Shadows
    float shadow = 1.0f;
    {
        // Find cascade
        int cascade_idx = -1;
        for (uint i = 0; i < min(g_ApplyLighting.cascades, MAX_NUM_CASCADES); ++i) 
        {
            if (view_depth <= g_ApplyLighting.cascadeSplits[i]) 
            {
                cascade_idx = i;
                break;
            }
        }

        if (cascade_idx >= 0)
        {
            // Shadow map coords
            float4 shadow_clip = mul(g_ApplyLighting.shadowVPs[cascade_idx], float4(world_pos, 1.0f));
            float3 shadow_ndc = shadow_clip.xyz / shadow_clip.w;

            // Convert from NDC [-1,1] to UV [0,1]
            float2 shadow_uv = float2(shadow_ndc.x * 0.5f + 0.5f,
                                      1.0f - (shadow_ndc.y * 0.5f + 0.5f));

            // Sample shadow map
            shadow = SampleShadowPCF(shadow_uv, shadow_ndc.z, cascade_idx, 0.001f);
        }
    }

    // Calculate lighting
    float4 final_color;
    {
        float specularity = 0.5f;
        float metallicness = 0.6f;
        float roughness = 0.4f;


        // Setup Directions
        float3 N = gbuf.normal;
        float3 V = normalize(GetCameraPos() - world_pos);
        float3 L = normalize(-g_ApplyLighting.light.direction); // Directional light

        // Setup Colors
        float3 albedo       = gbuf.diffColor.rgb;
        float3 dielectricF0 = 0.08f * specularity;
        float3 spec_color   = lerp(dielectricF0, albedo, metallicness);
        // Only non-metals have diffuse
        float3 diff_color   = albedo * (1.0f - metallicness);

        float3 diffuse_out;
        float3 specular_out;
        float3 ambient = albedo * 0.01f;

        // Calculate BRDF
        //GetBlinnPhongBRDF(N, V, L,
        //                  diff_color,
        //                  spec_color,
        //                  RoughnessToShininess(roughness),
        //                  g_ApplyLighting.light.color,
        //                  diffuse_out,
        //                  specular_out);
        GetDisneyBRDF(N, V, L,
                      diff_color,
                      spec_color,
                      roughness,
                      0.0f,
                      g_ApplyLighting.light.color,
                      diffuse_out, 
                      specular_out);

        // Apply Shadows and Combine
        diffuse_out *= shadow;
        specular_out *= shadow;

        float3 final_rgb = ambient + diffuse_out + specular_out;
        final_color = float4(final_rgb, 1.0f);
    }

    FetchRWTex2D(3).Store<float4>(screen_coord, final_color);
}