module ToneMap;
import Common;

ConstantBuffer<ToneMapCB> g_ToneMapCB;

struct Handles
{
    RWTexture2D<float4>.Handle output;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ToneMap(uint2 screen_coord : SV_DispatchThreadID, uniform Handles handles)
{
    float4 color = handles.output[screen_coord];

    // Apply gamma correction & brightness
    color.rgb = g_ToneMapCB.brightnessValue * pow(color.rgb, 1.0f / g_ToneMapCB.gammaValue);

    // Blend with clear color
    float4 blended;
    blended.rgb = lerp(g_ToneMapCB.clearColor.rgb, color.rgb, color.a);
    blended.a   = color.a + g_ToneMapCB.clearColor.a * (1.0f - color.a);

    handles.output[screen_coord] = blended;
}
