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
    };

    enum RenderTextureSize
    {
        RTSize_Full     = 0,
        RTSize_Half     = 1,

        RTSize_Count
    };

    enum RenderTextureFlag : uint32_t
    {
        RTFlag_None                    = 0,
        RTFlag_AllowRenderTarget       = (1 << 0), // Will not be used as a RenderTarget?
        RTFlag_AllowRandomGpuWrites    = (1 << 1), // Is UAV allowed?
        RTFlag_DenyAliasing            = (1 << 2), // Makes sure this resource is not shared between passes (likely contains history data)
        RTFlag_CustomSized             = (1 << 3), // Are the width & height properties using custom sizes?
        RTFlag_UiSized                 = (1 << 4), // Are the width & height properties based on UI sizes?
        RTFlag_UpscaledSized           = (1 << 5)  // Are the width & height properties based after the upscale?
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
            return ((flags & RTFlag_DenyAliasing) == 0 &&
                    (other.flags & RTFlag_DenyAliasing) == 0 &&
                    ((flags & RTFlag_CustomSized) == (other.flags & RTFlag_CustomSized)) &&
                    ((flags & RTFlag_UiSized) == (other.flags & RTFlag_UiSized)) &&
                    ((flags & RTFlag_UpscaledSized) == (other.flags & RTFlag_UpscaledSized)) &&
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
    };

    #define MAX_INOUT_RESOURCES_PER_RENDERPASS      8
    #define MAX_WORKING_RESOURCES_PER_RENDERPASS    8

    struct RenderPassConfig
    {
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