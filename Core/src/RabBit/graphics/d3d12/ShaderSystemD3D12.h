#pragma once

#include "RabBitCommon.h"
#include "graphics/ShaderSystem.h"

// If you get an error that this file cannot be found, run the ShaderCompilerD3D12 project
#include "codeGen/ShaderDefines.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <d3d12shader.h>
#include <dxcapi.h>

namespace RB::Graphics::D3D12
{
	struct CompiledShaderBlob
	{
		void*							shaderBlob;
		uint64_t						shaderBlobSize;
		GPtr<ID3D12ShaderReflection>	reflectionData;
	};

	class ShaderSystemD3D12 : public ShaderSystem
	{
	public:
		ShaderSystemD3D12();
		~ShaderSystemD3D12();

		void* GetCompilerShader(uint32_t shader_identifier) override;

		const ShaderResourceMask& GetShaderResourceMask(uint32_t shader_identifier) override;

	private:
		GPtr<IDxcUtils>		m_DxcUtils;
		CompiledShaderBlob*	m_ShaderBlobs[SHADER_ENTRIES];
		ShaderResourceMask	m_ShaderMasks[SHADER_ENTRIES];
	};
}