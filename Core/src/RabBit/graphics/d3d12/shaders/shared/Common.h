#if !SHADER
#include "RabBitCommon.h"

typedef RB::Math::Float2	float2;
typedef RB::Math::Float3	float3;
typedef RB::Math::Float4x4	float4x4;

#else

#define MAKE_REG_S(r)                     s##r
#define MAKE_REG_T(r)                     t##r
#define MAKE_REG_B(r)                     b##r
#define MAKE_REG_U(r)                     u##r

#define SAMPLER_REG(r)  register( MAKE_REG_S(r) )
#define TEXTURE_REG(r)  register( MAKE_REG_T(r) )
#define TBUFFER_REG(r)  register( MAKE_REG_T(r) )
#define CBUFFER_REG(r)  register( MAKE_REG_B(r) )
#define UBUFFER_REG(r)  register( MAKE_REG_U(r) )

#endif


// Global constant buffer slots
// -----------------------------------------

#define kFrameConstantsCB	0

#define kInstanceCB			1


// Global constant buffer structs
// -----------------------------------------

struct FrameConstants
{
	/*
		When passing matrix data to HLSL, it stores
		it in column major order. So make sure to
		first transpose matrices before sending them
		to the GPU as they are stored in row major
		on the CPU.
	*/

	float4x4 worldToViewMat;	// View matrix	(transposed)
	float4x4 viewToClipMat;		// Projection matrix
};

#if SHADER

// Global constant buffers
// -----------------------------------------

cbuffer FrameConstantsCB : CBUFFER_REG(kFrameConstantsCB)
{
	FrameConstants g_FC;
}

// Global helper functions
// -----------------------------------------

float4 TransformPosition(float3 position, float4x4 transform)
{
	return (position.xxxx * transform[0]) + (position.yyyy * transform[1]) + (position.zzzz * transform[2]) + transform[3];
}

float3 TransformLocalToWorld(float3 local_pos, float4x4 obj_to_world)
{
	return mul(obj_to_world, float4(local_pos, 1.0f)).xyz;
}

float3 TransformWorldToView(float3 world_pos)
{
	return mul(g_FC.worldToViewMat, float4(world_pos, 1.0f)).xyz;
}

float4 TransformViewToClip(float3 view_pos)
{
	return TransformPosition(view_pos, g_FC.viewToClipMat);
}

#endif