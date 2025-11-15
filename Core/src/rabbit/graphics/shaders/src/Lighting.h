#ifndef RB_SHADER_LIGHTING
#define RB_SHADER_LIGHTING

#include "../shared/ConstantBuffers.h"

// Blinn-Phong
// ---------------------------------------------------------------

void GetBlinnPhongDiffSpec(in float3  view_world_pos,
                           in float3  world_pos,
                           in float3  world_nrm,
                           in float   shininess,
                           in LightCB light,
                           out float3 diffuse,
                           out float3 specular)
{
    //float3 light_dir = normalize(light.worldPos - world_pos);
    float3 light_dir = normalize(-light.direction); // Directional light

    // diffuse
    float  diff      = max(dot(world_nrm, light_dir), 0.0f);
           diffuse   = diff * light.color;

    // specular
    float3 view_dir    = normalize(view_world_pos - world_pos);
    float3 halfway_dir = normalize(light_dir + view_dir);
    float  spec        = pow(max(dot(world_nrm, halfway_dir), 0.0f), shininess);
           specular    = spec * light.color;
}

#endif