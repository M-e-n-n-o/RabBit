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
#define PB

namespace RB::Graphics::Shader
{
#else
#define ALIGN_CHECK(type)
#define SIZE_EQUAL(type1, type2)
#define PB public
#endif

// Global constant buffer structs
// ---------------------------------------------------------------
PB struct FrameConstants
{
    PB float4x4 worldToViewMat;    // View matrix
    PB float4x4 viewToWorldMat;    // Inverse view matrix
    PB float4x4 viewToClipMat;     // Projection matrix
    PB float4x4 clipToViewMat;     // Inverse projection matrix
    PB float4   dimensions;        // width, height, 1/width, 1/height
};
ALIGN_CHECK(FrameConstants);

PB struct PresentCB
{
    PB float2 texOffset;
    PB float2 currSize;
    PB float  brightnessValue;
    PB float  gammaValue;
    PB uint   linearUpscale;
    PB float  padding;
};
ALIGN_CHECK(PresentCB);

PB struct DirectionalLight
{
    PB float3 direction;
    PB float  pad0;
    PB float3 color;
    PB float  pad1;
};

PB static const uint MAX_NUM_CASCADES = 4;

PB struct ApplyLightingCB
{
    PB float4x4            shadowVPs[MAX_NUM_CASCADES];
    PB float4              cascadeSplits; // A float4 because array's cause padding

    PB int                 cascades;
    PB float3              padding;

    PB DirectionalLight    light;
};
ALIGN_CHECK(ApplyLightingCB);

PB static const uint MIPS_PER_PASS = 4;

PB struct MipGeneratorCB
{
    PB uint2 size;     // Size of the start mip level
    PB uint  startMip; // The mip to read from
    PB uint  mipCount; // For this dispatch (needs to be <= MIPS_PER_PASS)
};
ALIGN_CHECK(MipGeneratorCB);

#if !SHADER
} // namespace RB::Graphics::Shader
#endif
#endif