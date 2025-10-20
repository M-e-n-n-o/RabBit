#ifndef RB_SHADER_RENDER_RESOURCES
#define RB_SHADER_RENDER_RESOURCES

// Descriptor slots/bindings
// ---------------------------------------------------------------
#define kStandardRegisterSpace      0 // This maps to the descriptor set in Vulkan

// VK bindings
#define kVkTex2DBinding             0
#define kVkRwTex2DBinding           1

#define kVkBindlessDescriptorSet    1

#if SHADER
#include "RenderResourceDefines.h"
#endif


// RenderResource wrappers
// ---------------------------------------------------------------

typedef uint RenderResourceHandle;

struct Tex2D
{
#if !SHADER
public:
#endif
    RenderResourceHandle handle;
    uint isSrgb;
    uint pad0;
    uint pad1;

#if SHADER
    bool IsSRGB()
    {
        return isSrgb;
    }

    template<typename TextureValueType>
    TextureValueType Sample(SamplerState ss, float2 uv)
    {
        Texture2D<TextureValueType> texture = GetResource<TextureValueType>();
        return texture.Sample(ss, uv);
    }
    
    template<typename TextureValueType>
    TextureValueType SampleLevel(SamplerState ss, uint2 coord, float level)
    {
        Texture2D<TextureValueType> texture = GetResource<TextureValueType>();
        return texture.SampleLevel(ss, coord, level);
    }

    template<typename TextureValueType>
    Texture2D<TextureValueType> GetResource()
    {
        return TEXTURE_DESCRIPTOR_HEAP(Texture2D, TextureValueType, handle);
    }
#endif
};
ALIGN_CHECK(Tex2D);

struct RwTex2D
{
#if !SHADER
public:
#endif
    RenderResourceHandle handle;
    uint pad0;
    uint pad1;
    uint pad2;

#if SHADER
    template<typename TextureValueType>
    TextureValueType Read(uint2 pos)
    {
        RWTexture2D<TextureValueType> texture = GetResource<TextureValueType>();
        return texture[pos];
    }

    template<typename TextureValueType>
    void Store(uint2 pos, TextureValueType value)
    {
        RWTexture2D<TextureValueType> texture = GetResource<TextureValueType>();
        texture[pos] = value;
    }

    template<typename TextureValueType>
    RWTexture2D<TextureValueType> GetResource()
    {
        return TEXTURE_DESCRIPTOR_HEAP(RWTexture2D, TextureValueType, handle);
    }
#endif
};
ALIGN_CHECK(RwTex2D);

#endif