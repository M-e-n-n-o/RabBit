#include "test2.hlsl"
#include "../shared/test2.h"

cbuffer ConstantBuffer : register(b0, space0)
{
    TransformCB g_Transform;
}

v2p VS_VertexColor(vertexInfo input)
{
    v2p output;
    output.position = float4(input.position + g_Transform.offset, 0, 1);
    output.color = input.color;
    return output;
}

cbuffer ConstantBuffer2 : register(b1, space0)
{
    ColorCB g_Color;
}

float4 PS_VertexColor(v2p input) : SV_TARGET
{
    return float4(g_Color.color, 1.0f);
}