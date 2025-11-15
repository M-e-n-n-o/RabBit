#ifndef RB_SHADER_TRANSFORM
#define RB_SHADER_TRANSFORM

#include "../shared/Common.h"

// --------------------------------------------------------------
float3 GetCameraDir()
{
    return float3(
        g_FC.viewToWorldMat[0].z,
        g_FC.viewToWorldMat[1].z,
        g_FC.viewToWorldMat[2].z
    );
}

// --------------------------------------------------------------
float3 GetCameraPos()
{
    return float3(
        g_FC.viewToWorldMat[0].w,
        g_FC.viewToWorldMat[1].w,
        g_FC.viewToWorldMat[2].w
    );
}

// --------------------------------------------------------------
float4 TransformPosition(float3 position, float4x4 transform)
{
    return (position.xxxx * transform[0]) + (position.yyyy * transform[1]) + (position.zzzz * transform[2]) + transform[3];
}

// --------------------------------------------------------------
float3 TransformLocalToWorld(float3 local_pos, float4x4 obj_to_world)
{
    return mul(obj_to_world, float4(local_pos, 1.0f)).xyz;
}

// --------------------------------------------------------------
float3 TransformWorldToView(float3 world_pos)
{
    return mul(g_FC.worldToViewMat, float4(world_pos, 1.0f)).xyz;
}

// --------------------------------------------------------------
float4 TransformViewToClip(float3 view_pos)
{
    return mul(g_FC.viewToClipMat, float4(view_pos.xyz, 1.0f));
}

// --------------------------------------------------------------
float3 TransformScreenUVsToWorld(float2 screen_uvs, float linear_depth, bool orthographic)
{
    float2 ndc = screen_uvs * 2.0f - 1.0f;

    float3 view_pos;
    if (orthographic)
    {
        float ortho_width = 2.0f / g_FC.viewToClipMat[0][0];
        float ortho_height = 2.0f / g_FC.viewToClipMat[1][1];

        view_pos = float3(ndc.x * ortho_width * 0.5f,
                          -ndc.y * ortho_height * 0.5f,
                          linear_depth);
    }
    else
    {
        float fx = g_FC.viewToClipMat[0][0];
        float fy = g_FC.viewToClipMat[1][1];

        view_pos = float3(ndc.x * linear_depth / fx,
                          -ndc.y * linear_depth / fy,
                          linear_depth);
    }

    float4 world_pos = mul(g_FC.viewToWorldMat, float4(view_pos, 1.0f));
    return world_pos.xyz;
}

// --------------------------------------------------------------
float2 TransformPixelCoordsToScreenUVs(uint2 coords)
{
    return (float2(coords) + 0.5f) * g_FC.dimensions.zw;
}

// --------------------------------------------------------------
bool IsReversedZ()
{
    return g_FC.viewToClipMat[2][2] < 0.0f;
}

// --------------------------------------------------------------
float2 ExtractNearFar(bool reversed_z)
{
    float C = g_FC.viewToClipMat[2][2];
    float D = g_FC.viewToClipMat[2][3];

    float near = -D / C;
    float far = (C * near) / (C - 1.0f);

    return reversed_z ? float2(far, near) : float2(near, far);
}

// --------------------------------------------------------------
float LinearizeDepth(float depth, float near, float far, bool reversed_z)
{
    if (reversed_z)
        return (near * far) / (depth * (far - near) + near);
    else
        return (near * far) / (far - depth * (far - near));
}

// --------------------------------------------------------------
float LinearizeDepth(float depth)
{
    bool reversed_z = IsReversedZ();
    float2 nf = ExtractNearFar(reversed_z);
    return LinearizeDepth(depth, nf.x, nf.y, reversed_z);
}

#endif