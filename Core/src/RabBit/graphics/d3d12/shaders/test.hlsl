struct vertexInfo
{
    float2 position : POSITION;
    float3 color    : COLOR;
};

struct v2p
{
    float4 position : SV_POSITION;
    float3 color    : TEXCOORD0;
};

v2p VS_VertexColor(vertexInfo input)
{
    v2p output;
    output.position = float4(input.position, 0, 1);
    output.color = input.color - 0.5f;
    return output;
}

float4 PS_VertexColor(v2p input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}