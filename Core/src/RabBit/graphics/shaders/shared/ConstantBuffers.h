#if !SHADER
#include "RabBitCommon.h"

typedef uint32_t			uint;

typedef RB::Math::UInt4		uint4;
typedef RB::Math::Float2	float2;
typedef RB::Math::Float3	float3;
typedef RB::Math::Float4	float4;
typedef RB::Math::Float4x4	float4x4;
#endif


struct PresentCB
{
	float2 texOffset;
	float2 currSize;
};