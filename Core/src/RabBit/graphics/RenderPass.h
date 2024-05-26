#pragma once

namespace RB::Graphics
{
	class ViewContext;
	class RenderInterface;

	enum class RenderPassType
	{
		None,
		GBuffer,
	};

	struct RenderPassConfig
	{
		RenderPassType		type;
		bool				onlyExecuteWhenDependedOn;
		RenderPassType		dependencies[8];
		//RenderTextureDesc	workingTextures[8];
		//RenderTextureDesc	outputTextures[8];
	};

	class RenderPassConfigBuilder
	{
		RenderPassConfigBuilder(const RenderPassType& type, bool only_execute_when_depended_on);

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

	typedef void RenderPassEntry;

	struct RenderPassContext
	{
		uint32_t entryCount;
		RenderPassEntry* entries;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;

		virtual RenderPassConfig GetConfiguration() = 0;

		// Executed on the main thread after the game logic update. This method just gives the needed context of the 
		// current frame' viewcontext to the renderpass (as the renderpass will run next frame as it is ~1 frame behind). 
		// This method will run for every ViewContext, so we can change things depening on the type of view context, 
		// lower quality for example. It can also do some preprocessing before the actual Render() call to, for example, 
		// determine which RenderEntries this pass needs, so the RenderThread does not need to do this. But it can maybe 
		// also determine if the pass needs to run at all even.
		virtual RenderPassContext SubmitContext(ViewContext* view_context) = 0;

		// Executed on the render thread
		//virtual void Render(RenderInterface* render_interface,
		//					ViewContext* view_context,
		//					RenderPassContext* context, 
		//					RenderTexture** output_textures, 
		//					RenderTexture** working_textures, 
		//					RenderTexture** dependency_textures) = 0;
	};
}