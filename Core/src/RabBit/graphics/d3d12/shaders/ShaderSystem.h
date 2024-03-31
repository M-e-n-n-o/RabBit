#pragma once

#include "RabBitCommon.h"
#include "codeGen/ShaderDefines.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <d3d12shader.h>
#include <dxcapi.h>

namespace RB::Graphics::D3D12
{
	struct ShaderBlob
	{
		void*							m_ShaderBlob;
		uint64_t						m_ShaderBlobSize;
		GPtr<ID3D12ShaderReflection>	m_ReflectionData;
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