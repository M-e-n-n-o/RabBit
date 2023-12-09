#pragma once

#include "RabBitCommon.h"

/*
	This file is just a planning file for the renderer for now
*/

namespace RB::Graphics
{
	struct TextureSize
	{
		bool isUsedForUI; // UI should always have the same dimensions as the real swapchain (should not have to be upscaled/downscaled)
		bool usesUpscaledRenderResolution;
		bool resizeWithRenderResolution;
		bool customSize;
		uint32_t customWidth;
		uint32_t customHeight;
	};

	enum class TextureFlags : uint32_t
	{
		None						= (1 << 0);
		ClearAtFrameStart			= (1 << 1);
		ClearAtRenderContextStart	= (1 << 2);
		ClearAtRenderPassStart		= (1 << 3);
		Keep1FrameHistory			= (1 << 4);
	};

	struct RenderTextureDesc
	{
		char* name;
		void* format;
		TextureSize size;
		uint32_t flags;

		// Maybe also add a fence value that indicates that this texture should wait on that fence value before being used by another commandlist again
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
		static RenderPassConfig GetConfiguration();

		// The return value is the output of this pass
		RenderTexture* Render(CommandList* commandList, RenderTexture** workingTextures, RenderPass** dependencies)
	};

	class RenderRect
	{
		float top;
		float bottom;
		float left;
		float right;
		float width;
		float height;
		float aspectRatio;
		float aspectRatioFull;
	};

	class Float4x4;

	class RenderContext
	{
		RenderPass* passes;	// A rendercontext should also be able to have overwrites (offsets of setting enums) of each RenderPass
		Float4x4 viewProjection;
		RenderRect renderRect; // Render location
		RenderTexture colorOutput;
		RenderTexture uiOutput;

		void Render()
		{
			// Render all passes
		}
	};

	class Renderer
	{
		RenderContext* contexts;

		void RenderContexts()
		{
			/*
				First render the different render contexts.
				After each context then something with the color and ui outputs has to be done, but what?
					- Merge the color and UI together and use as input for next render context (probably worst solution)
					- Copy the UI with the UI of the next render context
					- Maybe put in the renderpassconfig a thing to be able to receive optional previous contexts UI and/or color outputs as input

				After all contexts are rendered, first upscale the final colorOutput with the chosen upscaler, if any selected. After that, combine the upscaled color and the UI output into the "game backbuffer".
				This "game backbuffer" can have a different resolution than the swapchain backbuffer (if the user selects 1440p rendering with a 4k display while playing in fullscreen). After the merge,
				add a finalize path which "upscales/downscales", with a sampler in the pixel shader, to the actual swapchain backbuffer.
			*/
		}
	};
}