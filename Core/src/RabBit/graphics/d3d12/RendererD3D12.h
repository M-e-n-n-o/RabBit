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
        RendererD3D12(bool enable_validation_layer);
        ~RendererD3D12();

        void OnFrameStart() override;
        void OnFrameEnd() override;

        void SyncWithGpu() override;
    };
}