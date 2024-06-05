#include "test2.hlsl"

v2p VS_VertexColor(vertexInfo input)
{
    v2p output;
    output.position = float4(input.position, 0, 1);
    output.color = input.color;
    return output;
}

float4 PS_VertexColor(v2p input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}