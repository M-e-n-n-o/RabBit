#if RB_GRAPHICS_API_D3D12

#pragma once
#include "RabBitCommon.h"
#include "graphics/Renderer.h"
#include "graphics/Window.h"

#include <d3d12.h>

namespace RB::Graphics::D3D12
{
    #define MAX_NUM_CPU_FRAMES_IN_FLIGHT (BACK_BUFFER_COUNT + 1)
    #define TRANSIENT_CYCLES             MAX_NUM_CPU_FRAMES_IN_FLIGHT

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
#endif