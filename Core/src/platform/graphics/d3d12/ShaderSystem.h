#if RB_GRAPHICS_API_D3D12

#pragma once

#include "RabBitCommon.h"

// If you get an error that this file cannot be found, run the ShaderCompilerD3D12 project
#include <codeGen/ShaderDefines.h>
#include <codeGen/ShaderReflectionDefines.h>

#include <d3d12.h>
#include <d3d12shader.h>

namespace RB::Graphics::D3D12
{
    struct ShaderResourceMask
    {
        uint64_t cbvMask        = 0;
        uint64_t srvMask        = 0;
        uint64_t uavMask        = 0;
        uint64_t samplerMask    = 0;
    };

    struct CompiledShaderBlob
    {
        void*                           shaderBlob;
        uint64_t                        shaderBlobSize;
        GPtr<ID3D12ShaderReflection>    reflectionData;
    };

    class ShaderSystem
    {
    public:
        ShaderSystem();
        ~ShaderSystem();

        CompiledShaderBlob* GetCompilerShader(int32_t shader_identifier);

        const ShaderResourceMask& GetShaderResourceMask(int32_t shader_identifier);

    private:
        CompiledShaderBlob* m_ShaderBlobs[ShaderCompiler::SHADER_ENTRIES];
        ShaderResourceMask  m_ShaderMasks[ShaderCompiler::SHADER_ENTRIES];
        ShaderResourceMask  m_EmptyMask;
    };

    extern ShaderSystem* g_ShaderSystem;
}
#endif