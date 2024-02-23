#include "RabBitCommon.h"
#include "ShaderSystem.h"
#include <fstream>

namespace RB::Graphics::D3D12
{
	ShaderSystem* g_ShaderSystem = nullptr;

	ShaderSystem::ShaderSystem()
	{
		RB_ASSERT_FATAL_RELEASE_D3D(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_DxcUtils)), "Failed to create DXC Utils object");

		// Load the shader data from the binary file
		std::ifstream stream(SHADER_OBJ_FILE_LOCATION, std::ios::in | std::ios::binary);

		if (!stream.is_open())
		{
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Failed to open shader binary file");
			return;
		}

		for (uint64_t shader_index = 0; shader_index < SHADER_ENTRIES; ++shader_index)
		{
			ShaderBlob* blob = new ShaderBlob();

			// Read shader blob
			uint64_t start = SHADER_LUT[shader_index].offsetInFile;
			uint64_t end = SHADER_LUT[shader_index].offsetInFile + SHADER_LUT[shader_index].shaderBlobLength;

			stream.seekg(start, std::ios::beg);
			blob->m_ShaderBlobSize = end - start;
			blob->m_ShaderBlob = new char[blob->m_ShaderBlobSize];
			stream.read((char*)blob->m_ShaderBlob, blob->m_ShaderBlobSize);

			// Read reflection blob
			start = end;
			end += SHADER_LUT[shader_index].reflectionBlobLength;

			DxcBuffer reflection_data;
			reflection_data.Encoding	= DXC_CP_ACP;

			stream.seekg(start, std::ios::beg);
			reflection_data.Size = end - start;
			reflection_data.Ptr = new char[reflection_data.Size];
			stream.read((char*)reflection_data.Ptr, reflection_data.Size);

			// Load reflection data
			GPtr<ID3D12ShaderReflection> reflection;
			RB_ASSERT_FATAL_RELEASE_D3D(m_DxcUtils->CreateReflection(&reflection_data, IID_PPV_ARGS(&reflection)), "Failed to load reflection data of shader: %d", shader_index);

			m_ShaderBlobs[shader_index] = blob;
		}

		stream.close();
	}

	ShaderSystem::~ShaderSystem()
	{
		for (uint64_t shader_index = 0; shader_index < SHADER_ENTRIES; ++shader_index)
		{
			if (m_ShaderBlobs[shader_index] != nullptr)
			{
				delete m_ShaderBlobs[shader_index];
			}
		}
	}

	ShaderBlob* ShaderSystem::GetShaderBlob(uint32_t shader_identifier)
	{
		return m_ShaderBlobs[shader_identifier];
	}
}