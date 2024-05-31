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
			blob->shaderBlobSize = end - start;
			blob->shaderBlob = new char[blob->shaderBlobSize];
			stream.read((char*)blob->shaderBlob, blob->shaderBlobSize);

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
			RB_ASSERT_FATAL_RELEASE_D3D(m_DxcUtils->CreateReflection(&reflection_data, IID_PPV_ARGS(&blob->reflectionData)), "Failed to load reflection data of shader: %d", shader_index);

			delete[] reflection_data.Ptr;




			D3D12_SIGNATURE_PARAMETER_DESC desc;
			blob->reflectionData->GetInputParameterDesc(1, &desc);



			RB_LOG(LOGTAG_GRAPHICS, "Register: %d", desc.Register);
		}

		stream.close();
	}

	ShaderSystem::~ShaderSystem()
	{
		for (uint64_t shader_index = 0; shader_index < SHADER_ENTRIES; ++shader_index)
		{
			if (m_ShaderBlobs[shader_index] != nullptr)
			{
				delete[] m_ShaderBlobs[shader_index]->shaderBlob;
				delete m_ShaderBlobs[shader_index];
			}
		}
	}

	ShaderBlob* ShaderSystem::GetShaderBlob(uint32_t shader_identifier)
	{
		return m_ShaderBlobs[shader_identifier];
	}
}