#pragma once

#include "RenderResource.h"

namespace RB::Entity
{
    class Scene;
}

namespace RB::Graphics
{
    class ViewContext;
    class RenderResource;
    class RenderInterface;

    enum class RenderPassType
    {
        None,
        GBuffer,
        DeferredLighting,

        Count
    };

    enum RenderTextureSize
    {
        kRTSize_Full     = 0,
        kRTSize_Half     = 1,

        kRTSize_Count
    };

    enum RenderTextureFlag : uint32_t
    {
        kRTFlag_None                        = 0,
        kRTFlag_CustomSized                 = (1 << 0), // Are the width & height properties using custom sizes?
        kRTFlag_UiSized                     = (1 << 1), // Are the width & height properties based on UI sizes?
        kRTFlag_UpscaledSized               = (1 << 2),  // Are the width & height properties based after the upscale?
        kRTFlag_AllowRenderTarget           = (1 << 3), // Will not be used as a RenderTarget?
        kRTFlag_AllowRandomReadWrites       = (1 << 4), // Is UAV allowed?
        kRTFlag_DenyAliasing                = (1 << 5), // Makes sure this resource is not shared between passes (likely contains history data)
        kRTFlag_ClearBeforeGraph            = (1 << 6)  // Clears the resource to 0 before it enters the first RenderPass
    };

    struct RenderTextureDesc
    {
        const char*             name;
        RenderResourceFormat    format;
        uint32_t                width;  // RenderTextureSize
        uint32_t                height; // RenderTextureSize
        uint32_t                flags;

        bool IsAliasableWith(const RenderTextureDesc& other) const
        {
            return ((flags & kRTFlag_DenyAliasing) == 0 &&
                    (other.flags & kRTFlag_DenyAliasing) == 0 &&
                    ((flags & kRTFlag_CustomSized) == (other.flags & kRTFlag_CustomSized)) &&
                    ((flags & kRTFlag_UiSized) == (other.flags & kRTFlag_UiSized)) &&
                    ((flags & kRTFlag_UpscaledSized) == (other.flags & kRTFlag_UpscaledSized)) &&
                    format == other.format &&
                    width == other.width &&
                    height == other.height);
        }

        bool HasFlag(RenderTextureFlag flag) const
        {
            return (flags & flag) != 0;
        }

        void CombineFlags(const uint32_t other_flags)
        {
            // Just combine all the flags right now
            flags |= other_flags;
        }
    };

    struct RenderTextureInputDesc
    {
        const char* name;
        bool        depthFormat;
        // Points to an optional output texture (usefull for depth textures that we want to read from and write to)
        // If this is set, then the dependencyTexture stays nullptr and the RenderPass should use the outputTexture
        int32_t     outputTextureIndex;
    };

    #define MAX_INOUT_RESOURCES_PER_RENDERPASS      8
    #define MAX_WORKING_RESOURCES_PER_RENDERPASS    8

    struct RenderPassConfig
    {
        // TODO Remove the totalDependencies, totalWorkingTextures& totalOutputTextures indices, they are confusing and not very fool/me proof

        RenderTextureInputDesc	dependencies[MAX_INOUT_RESOURCES_PER_RENDERPASS];
        uint32_t                totalDependencies;
        RenderTextureDesc	    workingTextures[MAX_WORKING_RESOURCES_PER_RENDERPASS];
        uint32_t                totalWorkingTextures;
        RenderTextureDesc	    outputTextures[MAX_INOUT_RESOURCES_PER_RENDERPASS]; // Maybe it should be possible to not only output rendertextures, but also buffers?
        uint32_t                totalOutputTextures;
        bool                    asyncComputeCompatible  = false; // TODO Still unused
    };

    // Make sure to do all your deletes and free's in the destructor!
    struct RenderPassEntry
    {
        virtual ~RenderPassEntry() = default;
    };

    struct RenderPassSettings
    {
        virtual ~RenderPassSettings() = default;
    };

    struct RenderPassInput
    {
        ViewContext*     viewContext;
        RenderInterface* renderInterface;
        RenderPassEntry* entryContext;
        RenderResource** dependencyTextures;
        RenderResource** workingTextures;
        RenderResource** outputTextures;
    };

    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual const char* GetName() = 0;

        virtual RenderPassConfig GetConfiguration(const RenderPassSettings& settings) = 0;

        // Executed on the main thread after the game logic update. This method just gives the needed context of the 
        // current frame' viewcontext to the renderpass (as the renderpass will run next frame as it is ~1 frame behind). 
        // It can also do some preprocessing before the actual Render() call to, for example, determine which RenderEntries 
        // this pass needs, so the RenderThread does not need to do this. But it can maybe also determine if the pass needs 
        // to run at all even.
        virtual RenderPassEntry* SubmitEntry(const ViewContext* view_context, const Entity::Scene* const scene) = 0;

        // Executed on the render thread
        // Runs for every ViewContext
        virtual void Render(RenderPassInput& inputs) = 0;
    };
}