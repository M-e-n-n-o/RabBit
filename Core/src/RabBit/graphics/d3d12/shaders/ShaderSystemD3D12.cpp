#include "RabBitCommon.h"
#include "ShaderSystemD3D12.h"
#include <fstream>

namespace RB::Graphics::D3D12
{
	ShaderSystemD3D12::ShaderSystemD3D12()
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
			CompiledShaderBlob* blob = new CompiledShaderBlob();

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

			m_ShaderBlobs[shader_index] = blob;
			
			// Copy over shader masks
			m_ShaderMasks[shader_index].cbvMask = SHADER_LUT[shader_index].cbvMask;
			m_ShaderMasks[shader_index].srvMask = SHADER_LUT[shader_index].srvMask;
			m_ShaderMasks[shader_index].uavMask = SHADER_LUT[shader_index].uavMask;
			m_ShaderMasks[shader_index].samplerMask = SHADER_LUT[shader_index].samplerMask;
		}

		stream.close();
	}

	ShaderSystemD3D12::~ShaderSystemD3D12()
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

	void* ShaderSystemD3D12::GetCompilerShader(uint32_t shader_identifier)
	{
		return (void*) m_ShaderBlobs[shader_identifier];
	}
	
	const ShaderResourceMask& ShaderSystemD3D12::GetShaderResourceMask(uint32_t shader_identifier)
	{
		return m_ShaderMasks[shader_identifier];
	}
}