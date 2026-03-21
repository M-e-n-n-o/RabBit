module Simple;
import Common;
import Transform;

ConstantBuffer<float4x4> g_CustomMatrix;

struct PixelUniform
{
    Texture2D<float4>.Handle tex;
};

// Simple color
// ---------------------------------------------------------------

struct VI_Simple
{
    float2 position : POSITION;
    float4 color    : TEXCOORD0;
};

struct PI_SimpleCol
{
    float4 position : SV_POSITION;
    float4 color    : TEXCOORD0;
};

struct PI_Simple
{
    float4 position : SV_POSITION;
};

[shader("vertex")]
PI_SimpleCol Simple2D(VI_Simple input)
{
    PI_SimpleCol output;
    output.position = mul(g_CustomMatrix, float4(input.position, 1, 1));
    output.color    = input.color;

    return output;
}

[shader("pixel")]
float4 Simple2D(PI_SimpleCol input) : SV_Target0
{
    return input.color;
}

// Simple texture
// ---------------------------------------------------------------

struct VI_SimpleTex
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct PI_SimpleTex
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

[shader("vertex")]
PI_SimpleTex SimpleTex2D(VI_SimpleTex input)
{
    PI_SimpleTex output;
    output.position = mul(g_CustomMatrix, float4(input.position, 1, 1));
    output.uv = input.uv;

    return output;
}

[shader("pixel")]
float4 SimpleTex2D(PI_SimpleTex input, uniform PixelUniform u) : SV_Target0
{
    return u.tex.Sample(g_ClampAnisoSampler, input.uv);
}

// Simple 3D
// ---------------------------------------------------------------

struct VI_Simple3D
{
    float3 position : POSITION;
};

[shader("vertex")]
PI_Simple Simple3D(VI_Simple3D input)
{
    float3 world_pos = TransformLocalToWorld(input.position, g_CustomMatrix);
    float3 view_pos  = TransformWorldToView(world_pos);
    float4 clip_pos  = TransformViewToClip(view_pos);

    PI_Simple output;
    output.position = clip_pos;
    return output;
}