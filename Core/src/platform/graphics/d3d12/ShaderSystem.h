#pragma once

#include "RabBitCommon.h"

// If you get an error that this file cannot be found, run the ShaderCompilerD3D12 project
#include "codeGen/ShaderDefines.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <d3d12shader.h>
#include <dxcapi.h>

namespace RB::Graphics::D3D12
{
    struct ShaderResourceMask
    {
        uint64_t cbvMask;
        uint64_t srvMask;
        uint64_t uavMask;
        uint64_t samplerMask;
    };

    struct CompiledShaderBlob
    {
        void*                           shaderBlob;
        uint64_t						shaderBlobSize;
        GPtr<ID3D12ShaderReflection>	reflectionData;
    };

    class ShaderSystem
    {
    public:
        ShaderSystem();
        ~ShaderSystem();

        CompiledShaderBlob* GetCompilerShader(uint32_t shader_identifier);

        const ShaderResourceMask& GetShaderResourceMask(uint32_t shader_identifier);

    private:
        GPtr<IDxcUtils>		m_DxcUtils;
        CompiledShaderBlob* m_ShaderBlobs[SHADER_ENTRIES];
        ShaderResourceMask	m_ShaderMasks[SHADER_ENTRIES];
    };

    extern ShaderSystem* g_ShaderSystem;
}