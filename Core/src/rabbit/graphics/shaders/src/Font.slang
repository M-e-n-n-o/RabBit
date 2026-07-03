module Font;
import Common;

struct VI_Font
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct PI_Font
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

ConstantBuffer<float4x4> g_Projection;

[shader("vertex")]
PI_Font Font2D(VI_Font input)
{
    PI_Font output;
    output.position = mul(g_Projection, float4(input.position, 1, 1));
    output.uv = input.uv;

    return output;
}

struct PixelUniforms
{
    Texture2D<float>.Handle tex;
};

[shader("pixel")]
float4 Font2D(PI_Font input, uniform PixelUniforms u) : SV_Target0
{
    float alpha = u.tex.Sample(g_ClampAnisoSampler, input.uv);
    return float4(1.0, 1.0, 1.0, alpha);
}