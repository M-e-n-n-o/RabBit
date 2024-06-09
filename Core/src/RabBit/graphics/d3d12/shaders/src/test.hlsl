#include "test2.hlsl"
#include "../shared/test2.h"

cbuffer ConstantBuffer : register(b0, space0)
{
    TransformCB g_Transform;
}

v2p VS_VertexColor(vertexInfo input)
{
    v2p output;

    float4x4 mvp = mul(mul(g_Transform.localToWorldMat, g_Transform.worldToViewMat), g_Transform.viewToClipMat);

    output.position = mul(mvp, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}

//cbuffer ConstantBuffer2 : register(b1, space0)
//{
//    ColorCB g_Color;
//}

float4 PS_VertexColor(v2p input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}