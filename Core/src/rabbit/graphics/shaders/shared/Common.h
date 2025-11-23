#ifndef RB_SHADER_COMMON
#define RB_SHADER_COMMON

#if !SHADER
#include "RabBitCommon.h"

typedef uint32_t            uint;
typedef RB::Math::UInt2     uint2;
typedef RB::Math::UInt4     uint4;
typedef RB::Math::Float2    float2;
typedef RB::Math::Float3    float3;
typedef RB::Math::Float4    float4;
typedef RB::Math::Float4x4  float4x4;

#define ALIGN_CHECK(type)           static_assert(sizeof(type) % 16 == 0)
#define SIZE_EQUAL(type1, type2)    static_assert(sizeof(type1) == sizeof(type2))
#else
#define ALIGN_CHECK(type)
#define SIZE_EQUAL(type1, type2)
#endif


// Global slots
// ---------------------------------------------------------------

// Constant buffer slots
#define kRenderResourceMapCB        0
#define kFrameConstantsCB           1
#define kInstanceCB                 2

// Static samplers
#define kClampAnisoSamplerSlot      0
#define kClampPointSamplerSlot      1


// Global constant buffer structs
// ---------------------------------------------------------------

#include "RenderResources.h"

#define SHADER_RESOURCE_SLOTS 16

struct RenderResourceMap
{
    ShaderResource resources[SHADER_RESOURCE_SLOTS];
};

struct FrameConstants
{
    float4x4 worldToViewMat;    // View matrix
    float4x4 viewToWorldMat;    // Inverse view matrix
    float4x4 viewToClipMat;     // Projection matrix
    float4x4 clipToViewMat;     // Inverse projection matrix
    float4   dimensions;        // width, height, 1/width, 1/height
};

ALIGN_CHECK(RenderResourceMap);
ALIGN_CHECK(FrameConstants);

#if SHADER

// Global constant buffers
// ---------------------------------------------------------------

cbuffer RenderResourceMapCB : CBUFFER_REG(kRenderResourceMapCB)
{
    RenderResourceMap g_RenderResourceMap;
}

cbuffer FrameConstantsCB : CBUFFER_REG(kFrameConstantsCB)
{
    FrameConstants g_FC;
}

#define FetchTex2D(index)        ((Tex2D)g_RenderResourceMap.resources[index])
#define FetchTex2DArray(index)   ((Tex2DArray)g_RenderResourceMap.resources[index])
#define FetchRWTex2D(index)      ((RwTex2D)g_RenderResourceMap.resources[index])

// Static samplers
// ---------------------------------------------------------------

SamplerState g_ClampAnisoSampler : SAMPLER_REG(kClampAnisoSamplerSlot);
SamplerState g_ClampPointSampler : SAMPLER_REG(kClampPointSamplerSlot);

#endif

#endif