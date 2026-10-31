module ToneMap;
import Common;

ConstantBuffer<ToneMapCB> g_ToneMapCB;

// OpenDRT-style tonescale: m * (x / (x + s))^c, solved so that scene mid-grey (0.18) lands on
// the display grey level and scene value `x1` lands exactly on the display peak.
// All values are display-relative (1.0 == 100 nits).
float OpenDRT_Tonescale(float x, float contrast, float peakNits, float greyNits)
{
    const float x0 = 0.18f;                          // scene mid grey
    const float x1 = 16.0f * (peakNits / 100.0f);    // scene value that reaches peak (more headroom for HDR)
    const float y1 = peakNits / 100.0f;              // display peak
    const float y0 = greyNits / 100.0f;              // display grey

    // Solve s from f(x0) = y0 and f(x1) = y1
    float k = pow(y0 / y1, 1.0f / contrast);
    float s = x1 * x0 * (1.0f - k) / (k * x1 - x0);
    float m = y1 * pow((x1 + s) / x1, contrast);

    float y = m * pow(x / (x + s), contrast);
    return min(y, y1);
}

// ST.2084 (PQ) inverse EOTF for HDR10, input in absolute nits
float3 Encode_ST2084_PQ(float3 nits)
{
    const float m1 = 2610.0f / 16384.0f;
    const float m2 = 2523.0f / 4096.0f * 128.0f;
    const float c1 = 3424.0f / 4096.0f;
    const float c2 = 2413.0f / 4096.0f * 32.0f;
    const float c3 = 2392.0f / 4096.0f * 32.0f;

    float3 y  = pow(saturate(nits / 10000.0f), m1);
    return pow((c1 + c2 * y) / (1.0f + c3 * y), m2);
}

// Rec.709 -> Rec.2020 primaries (the scene is assumed to be linear Rec.709/sRGB)
float3 Rec709ToRec2020(float3 c)
{
    return float3(
        dot(c, float3(0.627404f, 0.329283f, 0.043313f)),
        dot(c, float3(0.069097f, 0.919540f, 0.011362f)),
        dot(c, float3(0.016391f, 0.088013f, 0.895595f)));
}

/**
 * @param scene     Scene-linear Rec.709 RGB.
 * @param isHDR     true: output HDR10 (PQ, Rec.2020). false: SDR (gamma encoded Rec.709).
 * @param peakNits  Display peak luminance (100 for SDR, e.g. 1000 for HDR).
 * @param gain      Linear brightness multiplier applied to the scene (1 = unchanged).
 * @param contrast  Tonescale contrast exponent (1.4 is the OpenDRT default).
 * @param gamma     SDR output gamma (2.2 typical, 1.0 for linear output).
 */
float3 ApplyOpenDRT(float3 scene, bool isHDR, float peakNits, float gain, float contrast, float gamma)
{
    float3 color = max(scene, 0.0f) * gain;

    float normIn = max(color.r, max(color.g, color.b));
    if (normIn <= 1e-6f)
        return 0.0f;

    // Mid grey stays at 10 nits for both SDR and HDR (OpenDRT convention)
    float peak = isHDR ? peakNits : 100.0f;
    float normOut = OpenDRT_Tonescale(normIn, contrast, peak, 10.0f);   // 1.0 == 100 nits

    // Hue preserving: scale RGB by the norm ratio (max channel never exceeds normOut)
    float3 rgb = color * (normOut / normIn);

    // Highlight dechroma: bright, saturated values roll off to the norm (white) as they approach peak
    float rel = normOut / (peak / 100.0f);                  // 0..1 of display peak
    float dechroma = smoothstep(0.25f, 1.0f, rel);
    rgb = lerp(rgb, normOut.xxx, dechroma);

    if (isHDR)
        return Encode_ST2084_PQ(Rec709ToRec2020(rgb) * 100.0f);
    else
        return pow(saturate(rgb), 1.0f / gamma);
}

struct Handles
{
    RWTexture2D<float4>.Handle output;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ToneMap(uint2 screen_coord : SV_DispatchThreadID, uniform Handles handles)
{
    float4 color = handles.output[screen_coord];

    // Blend with clear color
    float4 blended;
    blended.rgb = lerp(g_ToneMapCB.clearColor.rgb, color.rgb, color.a);
    blended.a   = color.a + g_ToneMapCB.clearColor.a * (1.0f - color.a);

    float3 tonemapped = ApplyOpenDRT(blended.rgb, g_ToneMapCB.isHDR != 0, g_ToneMapCB.peakLuminance, g_ToneMapCB.brightnessValue, g_ToneMapCB.contrast, g_ToneMapCB.gammaValue);

    handles.output[screen_coord] = float4(tonemapped, blended.a);
}
