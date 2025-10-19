#ifndef RB_SHADER_RENDER_RESOURCE_DEFINES
#define RB_SHADER_RENDER_RESOURCE_DEFINES

#if SHADER
#include "RenderResources.h"

// Register macro's
#define COMBINE_IMPL(a, b) a##b
#define COMBINE(a, b) COMBINE_IMPL(a, b)

#define SAMPLER_REG(r) register( COMBINE(s, r), COMBINE(space, kStandardRegisterSpace) )
#define TBUFFER_REG(r) register( COMBINE(t, r), COMBINE(space, kStandardRegisterSpace) )
#define CBUFFER_REG(r) register( COMBINE(b, r), COMBINE(space, kStandardRegisterSpace) )
#define TEXTURE_REG(r) register( COMBINE(t, r), COMBINE(space, kStandardRegisterSpace) )
#define UBUFFER_REG(r) register( COMBINE(u, r), COMBINE(space, kStandardRegisterSpace) )


// Descriptor heap macro's
#if SHADER_DX12

    #define TEXTURE_DESCRIPTOR_HEAP(resourceType, valueType, handle)    ResourceDescriptorHeap[NonUniformResourceIndex(handle)]
    #define BUFFER_DESCRIPTOR_HEAP(resourceType, handle)                ResourceDescriptorHeap[NonUniformResourceIndex(handle)]

#elif SHADER_VK
    // Vulkan currently does not have for support for directly indexing into the resource heap, so wrap around it

    // If we ever need more texture value types, we can add them below 
    // In Vulkan you can overlay resources on the same binding, make use of this by just defining every used value type of each texture type
    #define DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(texture_type, binding_a, binding_b)                                                                          \
                                                                                                                                                                \
            [[vk::binding(binding_a, binding_b)]] texture_type<float>                                                                                           \
                g##_##texture_type##float[];                                                                                                                    \
            [[vk::binding(binding_a, binding_b)]] texture_type<float4>                                                                                          \
                g##_##texture_type##float4[];                                                                                                                   \
                                                                                                                                                                \
            texture_type<float>  Get##texture_type##FromHeap(uint handle, float dummy) { return g##_##texture_type##float[NonUniformResourceIndex(handle)]; }   \
            texture_type<float4> Get##texture_type##FromHeap(uint handle, float4 dummy) { return g##_##texture_type##float4[NonUniformResourceIndex(handle)]; }

    DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(Texture2D,   kVkTex2DBinding,    kVkBindlessDescriptorSet)
    DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(RWTexture2D, kVkRwTex2DBinding,  kVkBindlessDescriptorSet)

    #define BUFFER_DESCRIPTOR_HEAP(resource_type, handle)               g##_##resource_type[NonUniformResourceIndex(handle)]
    #define TEXTURE_DESCRIPTOR_HEAP(resourceType, valueType, handle)    Get##resourceType##FromHeap(handle, (valueType)0)
#endif

#endif
#endif