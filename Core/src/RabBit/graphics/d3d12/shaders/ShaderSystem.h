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
	struct ShaderBlob
	{
		void*							shaderBlob;
		uint64_t						shaderBlobSize;
		GPtr<ID3D12ShaderReflection>	reflectionData;
	};

	class ShaderSystem
	{
	public:
		ShaderSystem();
		~ShaderSystem();

		ShaderBlob* GetShaderBlob(uint32_t shader_identifier);

	private:
		GPtr<IDxcUtils>		m_DxcUtils;
		ShaderBlob*			m_ShaderBlobs[SHADER_ENTRIES];
	};

	extern ShaderSystem* g_ShaderSystem;
}