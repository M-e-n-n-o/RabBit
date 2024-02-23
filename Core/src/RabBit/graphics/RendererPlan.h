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

	enum class RenderEntryType : uint32_t
	{
		3DObject		= (1 << 0);
		2DObject		= (1 << 1);
		Text			= (1 << 2);
		Volumetric		= (1 << 3);
	};

	// This is an abstract class, so a TextRenderEntry for example has its own class
	struct RenderEntry
	{
		RenderEntryType type;
	};

	enum class LightingType
	{
		Forward,
		Deferred
	};

	enum class Object3DFlags : uint32_t
	{
		None				= (1 << 0);
		SkipShadowCast		= (1 << 1);
		SkipShadowReceive	= (1 << 2);
	};

	struct RenderEntryObject3D : public RenderEntry
	{
		LightingType lightingType;
		bool hasTransparency;
		uint32_t flags;
	}

	enum class RenderPassType
	{
		GBuffer,
		AO,
		Shadow,
		Reflection
	};

	enum class RenderInterfaceType
	{
		Raster,
		Compute,
		Raytracing
	};

	class RenderInterface
	{
		// Standard render things are already in the base render interface, like copy

		void CopyResource();

		void SetShaderInput(uint32_t slot); // SRV
		void SetShaderOutput(uint32_t slot); // UAV
	};

	class RasterInterface : RenderInterface
	{
		void SetVertexData();
		void SetStencilValue();
		void SetRenderTexture();
		void Draw();
	};

	class ComputeInterface : RenderInterface
	{
		void Dispatch();
		// etc.
	};

	class RenderPass;

	// Some sort of builder class
	struct RenderPassConfig
	{
		// Define which render entry types this pass may receive, if it should only run when another pass needs it, which command list type it needs, and if it is async compute compatible (assert for only compute command lists!)
		RenderPassConfig(uint32_t renderEntryTypes, bool onlyExecutedWhenDependedOn, bool isAsyncCompatible, RenderInterfaceType interfaceType);

		void AddPassDependency(RenderPassType type);

		// Here you can add textures you need as input for the pass
		void AddWorkingTexture(RenderTextureDesc* desc);

		// Maybe it should also be possible to add multiple rendertexture outputs?
		// Or maybe not only possible to output rendertextures, but also buffers?
		void ConfigureOutput(RenderTextureDesc* desc);

		// Where will the settings of different renderpasses be, and how can passes access the settings of different passes?
		// - Maybe a renderpass can set the number of settings it can have (0 being the lowest setting).
		//   A context, or another pass, can then get/modify the setting by, for example, increasing to the next value.
	};

	class CommandList;
	class RenderTexture;

	class RenderPass
	{
		static RenderPassConfig GetConfiguration();

		void Render(RenderInterface* renderInterface, RenderTexture** outputTextures, RenderTexture** workingTextures, RenderPass** dependencies)
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

		void RenderContexts(RenderEntry* entries)
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