#include "RabBitCommon.h"
#include "ShaderSystem.h"
#include <fstream>

using namespace RB::ShaderCompiler;

namespace RB::Graphics
{
    ShaderSystem::ShaderSystem()
    {
        // Load the shader data from the binary file
        std::ifstream stream(ShaderCompiler::OBJ_FILE_LOCATION, std::ios::in | std::ios::binary);

        if (!stream.is_open())
        {
            RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Failed to open shader binary file");
            return;
        }

        for (uint64_t shader_index = 0; shader_index < ShaderCompiler::SHADER_ENTRIES; ++shader_index)
        {
            const ShaderReflection& lookup = ShaderCompiler::SHADERS_LOOKUP[shader_index];

            CompiledShaderBlob* blob = new CompiledShaderBlob();

            // Read shader blob
            uint64_t start = lookup.shaderBlob.offsetInFile;
            uint64_t end = start + lookup.shaderBlob.size;

            stream.seekg(start, std::ios::beg);
            blob->size = end - start;
            blob->blob = new char[blob->size];
            stream.read((char*)blob->blob, blob->size);
            m_ShaderBlobs[shader_index] = blob;
        }

        stream.close();

        RB_LOG(LOGTAG_GRAPHICS, "Loaded shader binary file");
    }

    ShaderSystem::~ShaderSystem()
    {
        for (uint64_t shader_index = 0; shader_index < ShaderCompiler::SHADER_ENTRIES; ++shader_index)
        {
            if (m_ShaderBlobs[shader_index] != nullptr)
            {
                delete[] m_ShaderBlobs[shader_index]->blob;
                delete m_ShaderBlobs[shader_index];
            }
        }
    }

    const CompiledShaderBlob* ShaderSystem::GetCompiledShader(int32_t shader_identifier) const
    {
        if (shader_identifier < 0 || shader_identifier >= ShaderCompiler::SHADER_ENTRIES)
            return nullptr;

        return m_ShaderBlobs[shader_identifier];
    }
    
    const ShaderCompiler::ShaderReflection* ShaderSystem::GetReflection(int32_t shader_identifier) const
    {
        if (shader_identifier < 0 || shader_identifier >= ShaderCompiler::SHADER_ENTRIES)
            return nullptr;

        return &ShaderCompiler::SHADERS_LOOKUP[shader_identifier];
    }
}