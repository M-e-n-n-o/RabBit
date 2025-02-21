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

    enum class RenderTextureSize : uint8_t
    {
        Full   = 0,
        Half   = 1
    };

    enum class RenderTextureFlag : uint32_t
    {
        None                    = 0,
        DenyRenderTarget        = (1 << 1),
        AllowRandomGpuWrites    = (1 << 2), // Is UAV allowed?
    };

    struct RenderTextureDesc
    {
        const char*             name;
        RenderResourceFormat    format;
        RenderTextureSize       width;
        RenderTextureSize       height;
        RenderTextureSize       depth;
        uint32_t                flags;
    };

    struct RenderTextureInputDesc
    {
        const char*             name;
        RenderResourceFormat    format;
    };

    struct RenderPassConfig
    {
        const char*             name;
        RenderPassType		    type;
        RenderTextureInputDesc	dependencies[8];
        uint32_t                totalDependencies;
        RenderTextureDesc	    workingTextures[8];
        uint32_t                totalWorkingTextures;
        RenderTextureDesc	    outputTextures[8]; // Maybe it should be possible to not only output rendertextures, but also buffers?
        uint32_t                totalOutputTextures;
    };

    // Make sure to do all your deletes and free's in the destructor!
    struct RenderPassEntry
    {
        virtual ~RenderPassEntry() = default;
    };

    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual RenderPassConfig GetConfiguration() = 0;

        // Executed on the main thread after the game logic update. This method just gives the needed context of the 
        // current frame' viewcontext to the renderpass (as the renderpass will run next frame as it is ~1 frame behind). 
        // It can also do some preprocessing before the actual Render() call to, for example, determine which RenderEntries 
        // this pass needs, so the RenderThread does not need to do this. But it can maybe also determine if the pass needs 
        // to run at all even.
        // 
        // TODO This method might have to run for every ViewContext, so we can change things depening on the type of view 
        // context, lower quality for example.
        virtual RenderPassEntry* SubmitEntry(const Entity::Scene* const scene) = 0;

        // Executed on the render thread
        // Runs for every ViewContext
        virtual void Render(RenderInterface* render_interface,
                            ViewContext*     view_context,
                            RenderPassEntry* entry_context,
                            RenderResource** output_textures,
                            RenderResource** working_textures,
                            RenderResource** dependency_textures) = 0;
    };
}