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
        Full            = 0,
        Half            = 1,
    };

    enum class RenderTextureFlag : uint32_t
    {
        None                    = 0,
        DenyRenderTarget        = (1 << 1), // Will not be used as a RenderTarget?
        AllowRandomGpuWrites    = (1 << 2), // Is UAV allowed?
        ContainsHistory         = (1 << 3)  // Makes sure this resource is not shared between passes
    };

    struct RenderTextureDesc
    {
        const char*             name;
        RenderResourceFormat    format;
        RenderTextureSize       width;
        RenderTextureSize       height;
        RenderTextureSize       depth;
        bool                    uiSized;
        bool                    upscaledSized;
        uint32_t                flags;
    };

    struct RenderTextureInputDesc
    {
        const char*             name;
        RenderResourceFormat    format;
    };

    enum class ViewportType : uint8_t
    {
        Standard    = 0,
        Upscaled    = 1,
        UI          = 2
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
        bool                    asyncComputeCompatible  = false;
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
        // 
        // TODO This method might have to run for every ViewContext, so we can change things depening on the type of view 
        // context, lower quality for example.
        virtual RenderPassEntry* SubmitEntry(const Entity::Scene* const scene) = 0;

        // Executed on the render thread
        // Runs for every ViewContext
        virtual void Render(RenderInterface* render_interface,
                            ViewContext*     view_context,
                            Viewport*        viewport,
                            RenderPassEntry* entry_context,
                            RenderResource** output_textures,
                            RenderResource** working_textures,
                            RenderResource** dependency_textures) = 0;
    };
}