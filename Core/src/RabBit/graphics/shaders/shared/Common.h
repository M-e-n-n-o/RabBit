#ifndef RB_SHADER_COMMON
#define RB_SHADER_COMMON

#if !SHADER
#include "RabBitCommon.h"

typedef uint32_t			uint;

typedef RB::Math::UInt2     uint2;
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
#define kTexIndicesCB			    0
#define kFrameConstantsCB		    1
#define kInstanceCB				    2

// Sampler slots
#define kClampAnisoSamplerSlot		0
#define kClampPointSamplerSlot		1


// Global constant buffer structs
// ---------------------------------------------------------------

#define SHADER_TEX2D_SLOTS		8

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
    float4x4 viewToClipMat;		// Projection matrix
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


// Global helper functions
// ---------------------------------------------------------------

float4 TransformPosition(float3 position, float4x4 transform)
{
    return (position.xxxx * transform[0]) + (position.yyyy * transform[1]) + (position.zzzz * transform[2]) + transform[3];
}

// ---------------------------
float3 TransformLocalToWorld(float3 local_pos, float4x4 obj_to_world)
{
    return mul(obj_to_world, float4(local_pos, 1.0f)).xyz;
}

// ---------------------------
float3 TransformWorldToView(float3 world_pos)
{
    return mul(g_FC.worldToViewMat, float4(world_pos, 1.0f)).xyz;
}

// ---------------------------
float4 TransformViewToClip(float3 view_pos)
{
    return TransformPosition(view_pos, g_FC.viewToClipMat);
}

// ---------------------------
uint PackUNorm(float input, uint shift, uint num_bits, float dither)
{
    uint mask = num_bits == 32 ? 0xFFFFFFFF : ((1U << num_bits) - 1);
    return uint(saturate(input) * float(mask) + dither) << shift;
}

// ---------------------------
float UnpackUNorm(uint enc_input, uint shift, uint num_bits)
{
    uint mask = num_bits == 32 ? 0xFFFFFFFF : ((1U << num_bits) - 1);
    enc_input = (enc_input >> shift) & mask;
    return float(enc_input) / float(mask);
}

// ---------------------------
float2 ExtractNearFar()
{
    float C = g_FC.viewToClipMat._33;
    float D = g_FC.viewToClipMat._43;

    float near = D / (C - 1.0);
    float far = D / C;

    if (near > far)
        return float2(far, near); // Inverted depth
    else
        return float2(near, far);
}

// ---------------------------
float LinearizeDepth(float depth, float near, float far)
{
    return near * far / (far - depth * (far - near));
}

#endif

#endif