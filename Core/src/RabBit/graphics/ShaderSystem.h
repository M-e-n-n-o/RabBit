#pragma once

// If you get an error that this file cannot be found, run the ShaderCompiler project for the graphics API you are using
#include "codeGen/ShaderDefines.h"

namespace RB::Graphics
{
	struct ShaderResourceMask
	{
		uint64_t cbvMask;
		uint64_t srvMask;
		uint64_t uavMask;
		uint64_t samplerMask;
	};

	class ShaderSystem
	{
	public:
		virtual ~ShaderSystem() = default;

		virtual void* GetCompilerShader(uint32_t shader_identifier) = 0;

		virtual const ShaderResourceMask& GetShaderResourceMask(uint32_t shader_identifier) = 0;

		static ShaderSystem* Create();

	protected:
		ShaderSystem() = default;
	};
}