#pragma once
#include "RabBitCommon.h"
#include "graphics/Renderer.h"
#include "DeviceQueue.h"

// DirectX 12 specific headers.
#include <d3d12.h>

namespace RB::Graphics::D3D12
{
	class RendererD3D12 : public Renderer
	{
	public:
		RendererD3D12();
		~RendererD3D12();

		virtual void OnFrameStart() override;
		virtual void OnFrameEnd() override;
	};
}