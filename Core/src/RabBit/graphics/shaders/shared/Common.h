#ifndef RB_SHADER_COMMON
#define RB_SHADER_COMMON

#if !SHADER
#include "RabBitCommon.h"

typedef uint32_t			uint;

typedef RB::Math::UInt2     uint2;
typedef RB::Math::UInt4		uint4;
typedef RB::Math::Float2	float2;
typedef RB::Math::Float3	float3;
typedef RB::Math::Float4    float4;
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
#define kTexIndicesCB			    0
#define kFrameConstantsCB		    1
#define kInstanceCB				    2

// Sampler slots
#define kClampAnisoSamplerSlot		0
#define kClampPointSamplerSlot		1


// Global constant buffer structs
// ---------------------------------------------------------------

#define SHADER_TEX2D_SLOTS          8

struct ShaderTexInfo
{
    uint  tableID;
    uint  isSRGB;
    uint2 padding;
};

struct TextureIndices
{
    ShaderTexInfo tex2D[SHADER_TEX2D_SLOTS];
    ShaderTexInfo rwTex2D[SHADER_TEX2D_SLOTS];
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
    float4x4 viewToWorldMat;    // Inverse view matrix
    float4x4 viewToClipMat;		// Projection matrix
    float4x4 clipToViewMat;     // Inverse projection matrix

    float4   dimensions;        // width, height, 1/width, 1/height
};

#if SHADER

// Global constant buffers
// ---------------------------------------------------------------

cbuffer TextureIndicesCB : CBUFFER_REG(kTexIndicesCB)
{
    TextureIndices g_TextureIndices;
}

cbuffer FrameConstantsCB : CBUFFER_REG(kFrameConstantsCB)
{
    FrameConstants g_FC;
}


// Global resource table
// ---------------------------------------------------------------

#define FetchRwTex2D(tex_id)    (ResourceDescriptorHeap[NonUniformResourceIndex(g_TextureIndices.rwTex2D[(tex_id)].tableID)])

Texture2D FetchTex2D(in uint tex_id, out bool is_srgb_space)
{
    ShaderTexInfo info = g_TextureIndices.tex2D[tex_id];
    is_srgb_space = info.isSRGB;
    return ResourceDescriptorHeap[NonUniformResourceIndex(info.tableID)];
}

Texture2D FetchTex2D(in uint tex_id)
{
    bool srgb;
    return FetchTex2D(tex_id, srgb);
}

// Global samplers
// ---------------------------------------------------------------

SamplerState g_ClampAnisoSampler : SAMPLER_REG(kClampAnisoSamplerSlot);
SamplerState g_ClampPointSampler : SAMPLER_REG(kClampPointSamplerSlot);

#endif

#endif