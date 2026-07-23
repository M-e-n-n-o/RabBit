#ifndef SHADER_SHARED
#define SHADER_SHADER

#if !SHADER
#include "RabBitCommon.h"

typedef uint32_t            uint;
typedef RB::Math::UInt2     uint2;
typedef RB::Math::UInt4     uint4;
typedef RB::Math::Float2    float2;
typedef RB::Math::Float3    float3;
typedef RB::Math::Float4    float4;
typedef RB::Math::Float4x4  float4x4;

#define PB

#else
#define PB public
#endif

PB static const uint MAX_COLLISION_CHECKS = 32;

PB struct CollisionCheckCB
{
    PB float4 points[MAX_COLLISION_CHECKS];
    PB uint maxPoints;
};

#endif