#pragma once
#include "../include/ShaderReflection.h"
#include <slang.h>
#include <slang-com-ptr.h>

class ShaderWriter
{
public:
	ShaderWriter();

	void WriteOutShaders(const std::string& defines_folder, const std::string& bin_folder,
		std::vector<RB::ShaderCompiler::CompiledShader>& shaders, const std::vector<Slang::ComPtr<slang::IBlob>>& blobs);
};