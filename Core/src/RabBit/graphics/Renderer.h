#pragma once

#include "RabBitCommon.h"

/*
	This file is just a planning file for the renderer for now
*/

namespace RB::Graphics
{
	struct TextureSize
	{
		bool usesUpscaledRenderResolution;
		bool resizeWithRenderResolution;
		bool customSize;
		uint32_t customWidth;
		uint32_t customHeight;
	};

	struct RenderTextureDesc
	{
		char* name;
		bool needsHistory; // If this is true, maybe then create 2 of these textures and ping pong between the two when giving it to the pass, so it can also read 1 frame back
		void* format;
		TextureSize size;
		uint32_t flags;
	};


	enum class RenderPassType
	{
		GBuffer,
		AO,
		Shadow,
		Reflection
	};

	class RenderPass;

	struct RenderPassConfig
	{
		RenderPassSetting(bool onlyExecutedWhenDependedOn, bool isAsyncCompatible, D3D12_COMMAND_LIST_TYPE commandListType);

		void AddPassDependency(RenderPassType type);

		// Here you can add textures you need as input for the pass
		void AddWorkingTexture(RenderTextureDesc* desc);

		// Maybe it should also be possible to add multiple rendertexture outputs?
		// Or maybe not only possible to output rendertextures, but also buffers?
		void ConfigureOutput(RenderTextureDesc* desc);

		// Where will the settings of different renderpasses be, and how can passes access the settings of different passes?
	};

	class CommandList;
	class RenderTexture;

	class RenderPass
	{
		static 
		static RenderPassConfig GetConfiguration();

		// The return value is the output of this pass
		RenderTexture* Render(CommandList* commandList, RenderTexture** workingTextures, RenderPass** dependencies)
	};

	class Renderer
	{
		void RenderFrame()
		{
			// First do render passes. These passes should not render to the swapchain backbuffer but use a "game backbuffer" which can have a different resolution
			// than the swapchain backbuffer. After the render passes, there will then be a composite pass, or finalize pass, which will "down-/upscale" it to the 
			// swapchain backbuffer resolution using a sampler in the pixel shader.
		}
	};
}