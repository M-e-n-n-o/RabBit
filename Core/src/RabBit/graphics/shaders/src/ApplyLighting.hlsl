#include "../shared/Common.h"
#include "Transform.h"
#include "Lighting.h"
#include "GBuffer.h"

[numthreads(8, 8, 1)]
void CS_ApplyLightingDeferred(uint2 screen_coord : SV_DispatchThreadID)
{
    GBufferTexIndices indices;
    indices.gbuf0 = 0;
    indices.gbuf1 = 1;

    float2 uv = TransformPixelCoordsToScreenUVs(screen_coord);

    GBuffer gbuf = SampleGBuffer(indices, uv);

    if (gbuf.depth <= 0.0001f)
    {
        // Bail out, nothing to light up
        return;
    }

    Light light;
    light.worldPos = GetCameraPos();
    light.color    = float3(1, 0, 0);

    float3 diffuse;
    float3 specular;
    GetBlinnPhongDiffSpec( GetCameraPos(),
                           TransformScreenUVsToWorld(uv, gbuf.depth),
                           gbuf.normal,
                           light,
                           diffuse,
                           specular);

    float3 ambient = gbuf.color.rgb * 0.05f;
    float4 final_color = float4(ambient + diffuse + specular, 1.0f);

    RWTexture2D<float4> output = FetchRwTex2D(0);

    output[screen_coord] = final_color;
}