#ifndef RB_SHADER_LIGHTING
#define RB_SHADER_LIGHTING

#include "../shared/Common.h"

// --------------------------------------------------------------
float3 GetCameraDir()
{
    return g_VP.m_ViewToWorldMat[2].xyz;
}

// --------------------------------------------------------------
float3 GetCameraPos()
{
    return g_VP.m_ViewToWorldMat[3].xyz;
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
float3 TransformScreenUVsToView(float2 screen_uvs, float linear_depth)
{
    // Scale and bias screen-space UVs into view-space coords
    float2 view_xy = screen_uvs * g_VP.m_ScreenToViewA.xy + g_VP.m_ScreenToViewA.zw;

    // Choose between attenuating XY by depth (projection) or not (orthographic)
    float depth_atten = linear_depth * g_VP.m_ScreenToViewB.x + g_VP.m_ScreenToViewB.y;

    // Combine into view-space position
    float3 vs_pos;
    vs_pos.z = linear_depth;
    vs_pos.xy = view_xy * depth_atten;

    return vs_pos;
}

// --------------------------------------------------------------
float2 TransformWorldToScreenUVs(float3 world)
{
    float4 clip_pos = TransformPosition(world - GetCameraPos(), g_VP.m_CamWorldToClipMat);

    return saturate((clip_pos.xy * rcp(clip_pos.w)) * float2(0.5f, -0.5f) + 0.5f);
}

// --------------------------------------------------------------
float3 TransformScreenUVsToWorld(float2 screen_uvs, float linear_depth)
{
    float3 vs_pos = TransformScreenUVsToView(screen_uvs, linear_depth);

    // Transform it to a camera-relative world-space position
    return mul(vs_pos, (float3x3)g_VP.m_ViewToWorldMat) + GetCameraPos();
}

// --------------------------------------------------------------
float2 TransformPixelCoordsToScreenUVs(uint2 coords)
{
    return (float2(coords) + 0.5f) * g_VP.m_InvDimensions.xy;
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