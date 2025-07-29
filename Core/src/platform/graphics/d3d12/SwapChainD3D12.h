#if RB_GRAPHICS_API_D3D12

#pragma once

#include "RabBitCommon.h"
#include "platform/windowing/SwapChain.h"
#include "graphics/RenderResource.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 extension library.
#include <d3dx12/d3dx12.h>

// Direct Composition, for transparency support
#include <dcomp.h>

namespace RB::Graphics::D3D12
{
    class SwapChainD3D12 : public SwapChain
    {
    public:
        SwapChainD3D12(HWND window_handle, const uint32_t width, const uint32_t height, const uint32_t buffer_count, RenderResourceFormat format, bool transparency_support);
        ~SwapChainD3D12();

        void Present(VsyncMode sync_mode) override;

        void Resize(const uint32_t width, const uint32_t height) override;

        GPtr<IDXGISwapChain4> Get4() const { return m_NativeSwapChain; }
        uint32_t GetWidth() override { return m_Width; }
        uint32_t GetHeight() override { return m_Height; }
        uint32_t GetBackBufferCount() override { return m_BackBufferCount; }
        uint32_t GetCurrentBackBufferIndex() override { return m_CurrentBackBufferIndex; }
        Graphics::Texture2D* GetCurrentBackBuffer() override;

    private:
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandleCPU(uint32_t back_buffer_index) const;

        void CreateDescriptorHeap();
        void UpdateRenderTargetViews();
        void CreateCompositionObjects(HWND window_handle);

        GPtr<IDXGISwapChain4>		m_NativeSwapChain;
        GPtr<ID3D12DescriptorHeap>	m_DescriptorHeap;
        GPtr<ID3D12Resource>*       m_BackBuffers;
        Graphics::Texture2D*        m_WrappedBackBuffers[BACK_BUFFER_COUNT];

        bool						m_UseComposition;
        GPtr<IDXGIDevice>			m_DeviceForComposition;
        GPtr<IDCompositionDevice>	m_CompositionDevice;
        GPtr<IDCompositionTarget>	m_CompositionTarget;

        uint32_t					m_DescriptorIncrementSize;
        uint32_t					m_CurrentBackBufferIndex;
        uint32_t					m_BackBufferCount;
        uint32_t					m_Width;
        uint32_t					m_Height;
        bool                        m_IsTearingSupported;
        RenderResourceFormat        m_EngineFormat;
    };
}
#endif