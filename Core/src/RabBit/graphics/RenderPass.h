#pragma once

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
        Streamer,
        GBuffer,
    };

    struct RenderPassConfig
    {
        const char*         friendlyName;
        RenderPassType		type;
        bool				onlyExecuteWhenDependedOn;
        RenderPassType		dependencies[8];
        //RenderTextureDesc	workingTextures[8];
        //RenderTextureDesc	outputTextures[8];
    };

    class RenderPassConfigBuilder
    {
    public:
        RenderPassConfigBuilder(const RenderPassType& type, const char* friendly_name, bool only_execute_when_depended_on);

        RenderPassConfigBuilder& AddDependency(RenderPassType type);

        // Here you can add textures you need as input for the pass
        // (aside from getting the outputs from the depending passes)
        //RenderPassConfigBuilder& AddWorkingTexture(RenderTextureDesc desc);

        // Maybe it should be possible to not only output rendertextures, but also buffers?
        //RenderPassConfigBuilder& AddOutput(RenderTextureDesc desc);

        RenderPassConfig Build();

    private:

        RenderPassConfig m_Config;
        uint32_t		 m_TotalDependencies;
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