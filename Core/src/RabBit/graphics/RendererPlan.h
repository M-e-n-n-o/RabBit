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
		ClearAtViewContextStart		= (1 << 2);
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

	class RenderInterface
	{
		void CopyResource();

		void SetShaderInput(uint32_t slot); // SRV
		void SetShaderOutput(uint32_t slot); // UAV

		void SetVertexData();
		void SetStencilValue();
		void SetRenderTexture();
		void Draw();

		void Dispatch();

		// etc.
	};

	class Mat4;
	class Light;

	class RenderPass;

	// Some sort of builder class
	struct RenderPassConfig
	{
		// Define if it should only run when another pass needs it, and if it is async compute compatible
		RenderPassConfig(char* name, bool onlyExecutedWhenDependedOn, bool isAsyncCompatible);

		void AddPassDependency(RenderPassType type);

		// Here you can add textures you need as input for the pass
		void AddWorkingTexture(RenderTextureDesc* desc);

		// Maybe it should also be possible to add multiple rendertexture outputs?
		// Or maybe not only possible to output rendertextures, but also buffers?
		void ConfigureOutput(RenderTextureDesc* desc);
	};

	class CommandList;
	class RenderTexture;

	// Contains most of the time RenderEntries, but maybe also cameraData and/or lights, or any info that the renderpass needs of the current frame really
	typedef void RenderPassContext;

	class ViewContext

	class RenderPass
	{
		static RenderPassConfig GetConfiguration();

		// Executed on the main thread after the game logic update
		// This method just gives the needed context of the current frame' viewcontext to the renderpass (as the renderpass will run next frame as it is 1 frame behind)
		// This method will run for every ViewContext, so we can change things depening on the type of view context, so lower quality for example.
		// Maybe does some preprocessing before the actual Render() call to, for example,
		// determine which RenderEntries this pass needs, so the RenderThread does not need to do this. 
		// But it can maybe also determine if the pass needs to run at all even (if AO for example is disabled and this is the AO pass, or if there are no things to render)
		RenderPassContext* ContextSubmit(ViewContext* viewContext);

		void Render(RenderPassContext* renderPassContext, RenderInterface* renderInterface, RenderTexture** outputTextures, RenderTexture** workingTextures, RenderPass** dependencies)
	};

	// Determines in which order which RenderPass is executed (the names are linked to the names given to RenderPassConfig::name)
	char* _RenderOrder
	{
		// First command list
		"GBuffer",
		"AO",
		"Reflection",
		"Shadow",
		"ApplyLighting",

		// Second command list
		"Upscale",
		"PostProcessing (ToneMap)",
		"UI"
	}

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

	enum class ViewContextType
	{
		BackBuffer,
		OffscreenHighQuality,
		OffscreenLowQuality
	};

	class ViewContext
	{
		ViewContextType type;
		Float4x4 viewProjection;
		RenderRect renderRect; // Render location
		RenderTexture output;
	};

	class Renderer
	{
		ViewContext* contexts;

		void RenderContexts(RenderEntry* entries)
		{
			/*
				First render the different render contexts (in order of the _RenderOrder).
				After each context then something with the color and ui outputs has to be done, but what?
					- Merge the color and UI together and use as input for next render context (probably worst solution)
					- Copy the UI with the UI of the next render context
					- Maybe put in the renderpassconfig a thing to be able to receive optional previous contexts UI and/or color outputs as input

				After all contexts are rendered, first upscale the final colorOutput with the chosen upscaler, if any selected. After that, combine the upscaled color and the UI output into the "game backbuffer".
				This "game backbuffer" can have a different resolution than the swapchain backbuffer (if the user selects 1440p rendering with a 4k display while playing in fullscreen). After the merge,
				add a finalize path which "upscales/downscales", with a sampler in the pixel shader, to the actual swapchain backbuffer.
			*/
		}

		// Where will the settings of different renderpasses be, and how can passes access the settings of different passes?
		// - Maybe a renderpass can set the number of settings it can have (0 being the lowest setting).
		//   A context, or another pass, can then get/modify the setting by, for example, increasing to the next value
		//	 (but how would you the  for example choose between XeGTAO or HBAO+, this is personal preference. Maybe a
		//   method index and quality index?).
		// - Changing rendersettings can also mean adding or removing renderpasses.
	};

	/*
		Very helpful presentation about using different commandlists and queues (also how many executecommandlists per frame)
		https://gpuopen.com/wp-content/uploads/2016/03/Practical_DX12_Programming_Model_and_Hardware_Capabilities.pdf

		Our plan:
		- We have one commandlist which will get filled and simply execute that in 1 executecommandlists call

		If we would in the future then see that there is down-time on the GPU between frames, look into the following

		Future improvements:
		- Maybe give each renderpass its own commandlist it can fill (this also means each renderpass on its own CPU thread)
		- Then finally call executecommandlists on 1 thread (this thread can maybe bundle a few commandlists together into 1
		  executecommandlists call so it can do work at the same time.
	*/
}