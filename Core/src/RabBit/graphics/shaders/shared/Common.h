#if !SHADER
#include "RabBitCommon.h"

typedef uint32_t			uint;

typedef RB::Math::UInt4		uint4;
typedef RB::Math::Float2	float2;
typedef RB::Math::Float3	float3;
typedef RB::Math::Float4x4	float4x4;

#else

#define COMBINE(a,b)		a##b

#define SAMPLER_REG(r)		register( COMBINE(s,r) )
#define TBUFFER_REG(r)		register( COMBINE(t,r) )
#define CBUFFER_REG(r)		register( COMBINE(b,r) )
#define TEXTURE_SPACE(s)	register( COMBINE(t,0), COMBINE(space,s) )
#define UBUFFER_SPACE(s)	register( COMBINE(u,0), COMBINE(space,s) )

#endif


// Global slots
// ---------------------------------------------------------------

// Constant buffer slots
#define kTexIndicesCB		0
#define kFrameConstantsCB	1
#define kInstanceCB			2

// Sampler slots
#define kClampSamplerSlot	0	

// Texture spaces
#define kTex2DTableSpace	0


// Global constant buffer structs
// ---------------------------------------------------------------

#define SHAED_TEX2D_ARRAY_SIZE	2
#define SHADER_TEX2D_SLOTS		(SHAED_TEX2D_ARRAY_SIZE * 4)

struct TextureIndices
{
	// Is in uint4 to avoid padding between elements in the array
	uint4 tex2D[SHAED_TEX2D_ARRAY_SIZE];
};

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
// ---------------------------------------------------------------

// TODO Maybe make this a Buffer (or ByteAddressBuffer)
cbuffer TextureIndicesCB : CBUFFER_REG(kTexIndicesCB)
{
	TextureIndices g_TextureIndices;
}

cbuffer FrameConstantsCB : CBUFFER_REG(kFrameConstantsCB)
{
	FrameConstants g_FC;
}



// Global resource tables (bindless)
// ---------------------------------------------------------------

Texture2D Texture2DTable[] : TEXTURE_SPACE(kTex2DTableSpace);
//TextureCube TextureCubeTable[] : registre(t0, kTexCubeTableSpace);
//RWTexture2D RwTexture2DTable[] : register(t0, kRwTex2DTableSpace);

static const uint g_TextureIndicesTex2D[SHADER_TEX2D_SLOTS] = (uint[SHADER_TEX2D_SLOTS])(g_TextureIndices.tex2D);

#define FETCH_TEX2D(index) (Texture2DTable[g_TextureIndicesTex2D[(index)]])



// Global samplers
// ---------------------------------------------------------------

SamplerState g_ClampSampler : SAMPLER_REG(kClampSamplerSlot);



// Global helper functions
// ---------------------------------------------------------------

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