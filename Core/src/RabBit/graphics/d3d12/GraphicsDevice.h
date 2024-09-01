#pragma once

#include "RabBitCommon.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>

// D3D12 extension library.
#include <d3dx12/d3dx12.h>

#include <d3d11.h>
#include <d3d11on12.h>

namespace RB::Graphics::D3D12
{
    class DeviceQueue;

    struct GraphicsCardInfo
    {
        const char* name;
        int64_t		videoMemory;
        int64_t		sharedSystemMemory;
    };

    class GraphicsDevice
    {
    public:
        GraphicsDevice(bool enable_debug_layer);
        ~GraphicsDevice();

        bool IsFormatSupported(DXGI_FORMAT format);
        bool IsFeatureSupported(DXGI_FEATURE feature);

        GPtr<ID3D12Device2> Get() const { return m_NativeDevice; }
        GPtr<IDXGIAdapter4> GetAdapter() const { return m_NativeAdapter; }
        GPtr<IDXGIFactory4> GetFactory() const { return m_Factory; }

        // Not recommended to use for regular rendering (currently only used for Direct Composition)
        GPtr<ID3D11On12Device> Get11On12();

        DeviceQueue* GetCopyQueue();
        DeviceQueue* GetComputeQueue();
        DeviceQueue* GetGraphicsQueue();

        void WaitUntilIdle();

        const GraphicsCardInfo GetGraphicsCardInfo() const { return m_GpuInfo; }

    private:
        void CreateFactory();
        void CreateAdapter();
        void CreateDevice(bool enable_debug_messages);

        GPtr<ID3D12Device2>			m_NativeDevice;
        GPtr<IDXGIAdapter4>			m_NativeAdapter;

        // Only created when required
        GPtr<ID3D11On12Device>		m_11On12Device;
        GPtr<ID3D11DeviceContext>	m_11DeviceContext;

        GPtr<IDXGIFactory4>			m_Factory;

        DeviceQueue*                m_CopyQueue;
        DeviceQueue*                m_ComputeQueue;
        DeviceQueue*                m_GraphicsQueue;

        GraphicsCardInfo			m_GpuInfo;
    };

    extern GraphicsDevice* g_GraphicsDevice;
}