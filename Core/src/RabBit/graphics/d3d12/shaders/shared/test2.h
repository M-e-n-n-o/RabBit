#if !SHADER
#include "RabBitCommon.h"

typedef RB::Math::Float2 float2;
typedef RB::Math::Float3 float3;

#else
#endif

struct TransformCB
{
	float2 offset;
};

struct ColorCB
{
	float3 color;
};