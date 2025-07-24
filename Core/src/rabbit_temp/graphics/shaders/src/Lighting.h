#ifndef RB_SHADER_LIGHTING
#define RB_SHADER_LIGHTING

#include "../shared/ConstantBuffers.h"

// Blinn-Phong
// ---------------------------------------------------------------

void GetBlinnPhongDiffSpec(in float3  view_world_pos,
                           in float3  world_pos,
                           in float3  world_nrm,
                           in Light   light,
                           out float3 diffuse,
                           out float3 specular)
{
    // diffuse
    float3 light_dir = normalize(light.worldPos - world_pos);
    float  diff      = max(dot(light_dir, world_nrm), 0.0);
           diffuse   = diff * light.color;

    // specular
    float3 view_dir    = normalize(view_world_pos - world_pos);
    float3 halfway_dir = normalize(light_dir + view_dir);
    float  spec        = pow(max(dot(world_nrm, halfway_dir), 0.0), 64.0);
           specular    = spec * light.color;
}

#endif