#pragma once

#include "RabBitCommon.h"
#include "graphics/Window.h"

namespace RB::Graphics
{
    class SwapChain
    {
    public:
        virtual ~SwapChain() = default;

        virtual void Present() = 0;

        virtual void Resize(const uint32_t wdith, const uint32_t height) = 0;

        virtual uint32_t GetWidth() = 0;
        virtual uint32_t GetHeight() = 0;

        virtual uint32_t GetBackBufferCount() = 0;
        virtual uint32_t GetCurrentBackBufferIndex() = 0;
        virtual Graphics::Texture2D* GetCurrentBackBuffer() = 0;

        virtual void* GetNativeSwapChain() const = 0;
    };
}