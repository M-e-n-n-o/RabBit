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

[shader("vertex")]
PI_Font VS_Font2D(VI_Font input, uniform float4x4 projection)
{
    PI_Font output;
    output.position = mul(projection, float4(input.position, 1, 1));
    output.uv = input.uv;

    return output;
}

[shader("pixel")]
float4 PS_Font2D(PI_Font input, uniform Texture2D<float>.Handle tex) : SV_Target0
{
    float alpha = tex.Sample(g_ClampAnisoSampler, input.uv);
    return float4(1.0, 1.0, 1.0, alpha);
}