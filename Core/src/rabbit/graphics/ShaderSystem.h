#pragma once

// If you get an error that this file cannot be found, run the ShaderCompiler project
#include <codeGen/ShaderDefines.h>
#include <codeGen/ShaderReflectionDefines.h>

namespace RB::Graphics
{
    struct CompiledShaderBlob
    {
        void*       blob;
        uint64_t    size;
    };

    class ShaderSystem
    {
    public:
        ShaderSystem();
        ~ShaderSystem();

        const CompiledShaderBlob* GetCompiledShader(int32_t shader_identifier) const;

        const ShaderCompiler::ShaderReflection* GetReflection(int32_t shader_identifier) const;

    private:
        CompiledShaderBlob* m_ShaderBlobs[ShaderCompiler::SHADER_ENTRIES];
    };
}