#if !SHADER
#include "RabBitCommon.h"

typedef RB::Math::Float2	float2;
typedef RB::Math::Float3	float3;
typedef RB::Math::Float4x4	float4x4;

#else
#endif

struct TransformCB
{
	// Matrices are processed in column major in HLSL! (by default)

	float4x4 localToWorldMat;
	float4x4 worldToViewMat;
	float4x4 viewToClipMat;
};

struct ColorCB
{
	float3 color;
};