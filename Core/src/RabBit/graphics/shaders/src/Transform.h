#ifndef RB_SHADER_TRANSFORM
#define RB_SHADER_TRANSFORM

#include "../shared/Common.h"

// --------------------------------------------------------------
float3 GetCameraDir()
{
    return g_FC.viewToWorldMat[2].xyz;
}

// --------------------------------------------------------------
float3 GetCameraPos()
{
    return g_FC.viewToWorldMat[3].xyz;
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
    return TransformPosition(view_pos, g_FC.viewToClipMat);
}

// --------------------------------------------------------------
float3 TransformScreenUVsToWorld(float2 screen_uvs, float linear_depth)
{
    float2 ndc = screen_uvs * 2.0f - 1.0f;

    float4 clip_pos = float4(ndc.x, ndc.y, 1.0f, 1.0f);

    // Transform to view space
    float4 view_dir = mul(clip_pos, g_FC.clipToViewMat);
    view_dir.xyz /= view_dir.w;

    float3 view_pos = normalize(view_dir.xyz) * linear_depth;

    // Transform to world space
    float4 world_pos = mul(float4(view_pos, 1.0f), g_FC.viewToWorldMat);

    return world_pos.xyz;
}

// --------------------------------------------------------------
float2 TransformPixelCoordsToScreenUVs(uint2 coords)
{
    return (float2(coords) + 0.5f) * g_FC.dimensions.zw;
}

// --------------------------------------------------------------
float2 ExtractNearFar()
{
    float C = g_FC.viewToClipMat._33;
    float D = g_FC.viewToClipMat._43;

    float near = -D / C;
    float far = (C * near) / (C - 1.0f);

    return float2(near, far);
}

// --------------------------------------------------------------
float LinearizeDepth(float depth, float near, float far)
{
    if (near > far)
        return near * far / (far + depth * (near - far));
    else
        return (near * far) / (far - depth * (far - near));
}

// --------------------------------------------------------------
float LinearizeDepth(float depth)
{
    float2 nf = ExtractNearFar();
    return LinearizeDepth(depth, nf.x, nf.y);
}

#endif