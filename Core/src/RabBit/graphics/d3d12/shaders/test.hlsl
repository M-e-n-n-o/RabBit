
struct vertexInfo
{
    float2 position : POSITION;
    float3 color    : COLOR;
}

struct v2p
{
    float4 position : SV_POSITION;
    float3 color    : TEXCOORD0;
}

v2p vs_main(vertexInfo input)
{
    v2p output;
    output.position = float4(input.position, 0, 1);
    output.color = input.color;
    return output;
}

float4 ps_main(v2p input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}