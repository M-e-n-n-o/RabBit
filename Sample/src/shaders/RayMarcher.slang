module RayMarcher;
import Common;
import GBuffer;
import Transform;

#include "Shared.h"

#define MAX_DIST 500
#define MAX_STEPS 50
#define SURFACE_DIST 0.001f

float Sphere(float3 p, float radius) 
{
    return length(p) - radius;
}

float Mandelbulb(float3 p)
{
    float3 z = p;
    float dr = 1.0f;
    float r = 0.0f;
    float power = 8.0f; // Standard Mandelbulb power

    for (int i = 0; i < 10; i++)
    {
        r = length(z);
        if (r > 2.0f) 
            break; // Bailout condition

        // Convert to spherical coordinates and scale by power
        float theta = acos(z.z / max(r, 0.0001f)) * power; 
        float phi = atan2(z.y, z.x) * power;
        
        // Derivative estimation for distance field scaling
        dr = pow(r, power - 1.0f) * power * dr + 1.0f;

        // Convert back to Cartesian coordinates and add original position
        float sinTheta, cosTheta;
        sincos(theta, sinTheta, cosTheta);
        
        float sinPhi, cosPhi;
        sincos(phi, sinPhi, cosPhi);

        z = pow(r, power) * float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta) + p;
    }
    return 0.5f * log(r) * r / dr; // Distance estimate
}

float Scene(float3 p)
{
    float distance = Sphere(p - float3(0, 20, 20), 10.0f);
    //distance = min(Mandelbulb(p), distance);
    return distance;
}

float RayMarch(float3 ro, float3 rd, float max_depth)
{
    float dO = 0.0f;

    for (int step = 0; step < MAX_STEPS; step++)
    {
        float3 p = ro + rd * dO;
        float ds = Scene(p);

        dO += ds;

        if (dO > max_depth || ds < SURFACE_DIST)
        {
            break;
        }
    }

    return dO;
}

float3 GetNormal(float3 p) 
{
  float2 e = float2(0.01f, 0);

  float3 n = Scene(p) - float3(Scene(p - e.xyy),
                               Scene(p - e.yxy),
                               Scene(p - e.yyx));

  return normalize(n);
}

struct Handles
{
    GBufferRwTexHandles gbuffer;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void RayMarching(uint2 screen_coord : SV_DispatchThreadID, uniform Handles handles)
{
    GBuffer gbuf = SampleRwGBuffer(handles.gbuffer, screen_coord);

    float2 uv = TransformPixelCoordsToScreenUVs(screen_coord);

    float3 ro      = GetCameraPos();
    float3 world_p = TransformScreenUVsToWorld(uv, 1.0f, false);
    float3 rd      = normalize(world_p - ro);
    float3 rd_view = normalize(TransformWorldToView(world_p));

    float max_linear_depth = (gbuf.depth > 0.0001f) ? min(gbuf.depth, MAX_DIST) : MAX_DIST;
    float max_ray_distance = max_linear_depth / rd_view.z;

    float d = RayMarch(ro, rd, max_ray_distance);

    if (d < max_ray_distance)
    {
        float3 p = ro + rd * d;
        float4 clip = TransformViewToClip(TransformWorldToView(p));

        gbuf.diffColor = float4(1.0f);
        gbuf.normal = GetNormal(p);
        gbuf.depth = LinearizeDepth(clip.z / clip.w);

        WriteGBuffer(handles.gbuffer, gbuf, screen_coord);
    }
}

struct CollisionHandles
{
    RWStructuredBuffer<float>.Handle distances;
}

ConstantBuffer<CollisionCheckCB> g_CollisionChecks;

[shader("compute")]
[numthreads(MAX_COLLISION_CHECKS, 1, 1)]
void CollisionCheck(uint coord : SV_DispatchThreadID, uniform CollisionHandles handles)
{
    if (coord >= g_CollisionChecks.maxPoints)
    {
        return;
    }

    float3 p = g_CollisionChecks.points[coord].xyz;
    handles.distances[coord] = Scene(p);
}