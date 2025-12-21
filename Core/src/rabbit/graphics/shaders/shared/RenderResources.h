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

// This struct should be casted to the implementation structs
struct ShaderResource
{
    RenderResourceHandle handle;
    uint data0;
    uint data1;
    uint data2;
};

struct Tex2D
{
    RenderResourceHandle handle;
    uint pad0;
    uint pad1;
    uint pad2;

#if SHADER
    template<typename TextureValueType>
    TextureValueType Sample(SamplerState ss, float2 uv)
    {
        Texture2D<TextureValueType> texture = GetResource<TextureValueType>();
        return texture.Sample(ss, uv);
    }

    template<typename TextureValueType>
    TextureValueType SampleLevel(SamplerState ss, float2 uv, float level)
    {
        Texture2D<TextureValueType> texture = GetResource<TextureValueType>();
        return texture.SampleLevel(ss, uv, level);
    }

    template<typename TextureValueType>
    TextureValueType Load(int3 coord)
    {
        Texture2D<TextureValueType> texture = GetResource<TextureValueType>();
        return texture.Load(coord);
    }

    template<typename TextureValueType>
    void GetDimensions(out float width, out float height)
    {
        Texture2D<TextureValueType> texture = GetResource<TextureValueType>();
        texture.GetDimensions(width, height);
    }

    template<typename TextureValueType>
    Texture2D<TextureValueType> GetResource()
    {
        return TEXTURE_DESCRIPTOR_HEAP(Texture2D, TextureValueType, handle);
    }
#endif
};
SIZE_EQUAL(Tex2D, ShaderResource);

struct Tex2DArray
{
    RenderResourceHandle handle;
    uint pad0;
    uint pad1;
    uint pad2;

#if SHADER    
    template<typename TextureValueType>
    TextureValueType Sample(SamplerState ss, float3 uv)
    {
        Texture2DArray<TextureValueType> texture = GetResource<TextureValueType>();
        return texture.Sample(ss, uv);
    }

    template<typename TextureValueType>
    void GetDimensions(out float width, out float height, out float elements)
    {
        Texture2DArray<TextureValueType> texture = GetResource<TextureValueType>();
        texture.GetDimensions(width, height, elements);
    }

    template<typename TextureValueType>
    Texture2DArray<TextureValueType> GetResource()
    {
        return TEXTURE_DESCRIPTOR_HEAP(Texture2DArray, TextureValueType, handle);
    }
#endif
};
SIZE_EQUAL(Tex2DArray, ShaderResource);

struct RwTex2D
{
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
SIZE_EQUAL(RwTex2D, ShaderResource);

#endif