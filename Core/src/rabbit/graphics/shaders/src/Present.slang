module Present;
import Common;

struct vertexInfo
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct v2p
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

ConstantBuffer<PresentCB> g_PresentCB;

[shader("vertex")]
v2p Present(vertexInfo input)
{
    v2p output;
    output.position = float4(input.position, 0, 1);
    output.uv       = input.uv;

    return output;
}

struct PixelUniforms
{
    Texture2D<float4>.Handle tex;
};

[shader("pixel")]
float4 Present(v2p input, uniform PixelUniforms u) : SV_TARGET
{
    float2 screen_coords = input.uv * g_PresentCB.currSize;
    float2 tex_coords    = screen_coords - g_PresentCB.texOffset.xy;
    float2 tex_uv        = tex_coords / (g_PresentCB.currSize - (g_PresentCB.texOffset.xy * 2.0f));

    if (tex_uv.x < 0.0f || tex_uv.x > 1.0f || tex_uv.y < 0.0f || tex_uv.y > 1.0f)
    {
        return 0.0f;
    }

    float4 color;
    if (g_PresentCB.linearUpscale)
        color = u.tex.Sample(g_ClampLinearSampler, tex_uv);
    else
        color = u.tex.Sample(g_ClampPointSampler, tex_uv);

    return color;
}