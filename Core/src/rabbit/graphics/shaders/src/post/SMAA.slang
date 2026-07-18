module SMAAEdgeDetect;
import Common;

#define SMAA_RT_METRICS float4(g_FC.dimensions.z, g_FC.dimensions.w, g_FC.dimensions.x, g_FC.dimensions.y)
#define SMAA_PRESET_HIGH
#define SMAA_CUSTOM_SL

#ifdef SMAA_CUSTOM_SL
    #define SMAATexture2D(tex)                              Texture2D tex
    #define SMAATexturePass2D(tex)                          tex
    #define SMAASampleLevelZero(tex, coord)                 tex.SampleLevel(g_ClampLinearSampler, coord, 0.0f)
    #define SMAASampleLevelZeroPoint(tex, coord)            tex.SampleLevel(g_ClampPointSampler, coord, 0.0f)
    #define SMAASampleLevelZeroOffset(tex, coord, offset)   tex.SampleLevel(g_ClampLinearSampler, coord, 0.0f, offset)
    #define SMAASample(tex, coord)                          tex.Sample(g_ClampLinearSampler, coord)
    #define SMAASamplePoint(tex, coord)                     tex.Sample(g_ClampPointSampler, coord)
    #define SMAASampleOffset(tex, coord, offset)            tex.Sample(g_ClampLinearSampler, coord, offset)
    #define SMAATexture2DMS2(tex)                           Texture2DMS<float4, 2> tex
    #define SMAALoad(tex, pos, sample)                      tex.Load(pos, sample)
    #define SMAAGather(tex, coord)                          tex.Gather(g_ClampLinearSampler, coord)
    #define SMAA_FLATTEN [flatten]
    #define SMAA_BRANCH  [branch]
#endif

#include "SMAA.hlsl"

struct VI
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
};

// ----------------------------------------------------------------------------
// Edge Detection

struct PI_EDGE_DETECT
{
    float4 position  : SV_POSITION;
    float2 uv        : TEXCOORD0;
    float4 offset[3] : TEXCOORD1;
};

[shader("vertex")]
PI_EDGE_DETECT SmaaEdgeDetection(VI i)
{
    PI_EDGE_DETECT o;
    o.position = float4(i.position, 0.0f, 1.0f);
    o.uv = i.uv;
    SMAAEdgeDetectionVS(i.uv, o.offset);

    return o;
}

struct EdgeDetectPixelInput
{
    Texture2D.Handle colorTex;
};

[shader("pixel")]
float2 SmaaEdgeDetection(PI_EDGE_DETECT i, uniform EdgeDetectPixelInput u) : SV_TARGET
{
    return SMAALumaEdgeDetectionPS(i.uv, i.offset, u.colorTex);
}

// ----------------------------------------------------------------------------
// Blending weight calculation

struct PI_BLEND_WEIGHT
{
    float4 position  : SV_POSITION;
    float2 uv        : TEXCOORD0;
    float2 pixCoord  : TEXCOORD1;
    float4 offset[3] : TEXCOORD2;
};

[shader("vertex")]
PI_BLEND_WEIGHT SmaaBlendWeight(VI i)
{
    PI_BLEND_WEIGHT o;
    o.position = float4(i.position, 0.0f, 1.0f);
    o.uv = i.uv;
    SMAABlendingWeightCalculationVS(i.uv, o.pixCoord, o.offset);

    return o;
}

struct BlendWeightPixelInput
{
    Texture2D.Handle edgesTex;
    Texture2D.Handle areaTex;
    Texture2D.Handle searchTex;
};

[shader("pixel")]
float4 SmaaBlendWeight(PI_BLEND_WEIGHT i, uniform BlendWeightPixelInput u) : SV_TARGET
{
    return SMAABlendingWeightCalculationPS(i.uv, i.pixCoord, i.offset, u.edgesTex, u.areaTex, u.searchTex, 0);
}

// ----------------------------------------------------------------------------
// Neighborhood blending

struct PI_NEIGHBOR_BLEND
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float4 offset   : TEXCOORD2;
};

[shader("vertex")]
PI_NEIGHBOR_BLEND SmaaNeighborBlending(VI i)
{
    PI_NEIGHBOR_BLEND o;
    o.position = float4(i.position, 0.0f, 1.0f);
    o.uv = i.uv;
    SMAANeighborhoodBlendingVS(i.uv, o.offset);

    return o;
}

struct NeighborBlendPixelInput
{
    Texture2D.Handle colorTex;
    Texture2D.Handle blendTex;
};

[shader("pixel")]
float4 SmaaNeighborBlending(PI_NEIGHBOR_BLEND i, uniform NeighborBlendPixelInput u) : SV_TARGET
{
    return SMAANeighborhoodBlendingPS(i.uv, i.offset, u.colorTex, u.blendTex);
}