#pragma once
#include "Compiler.h"
#include "../include/ShaderReflection.h"
#include <set>
#include <slang.h>
#include <slang-com-ptr.h>

class ShaderWriter
{
public:
	ShaderWriter();

	void WriteOutShaders(const char* defines_folder, const char* bin_folder,
		std::vector<RB::ShaderCompiler::ShaderReflection>& shaders, const std::vector<Slang::ComPtr<slang::IBlob>>& blobs, const ModuleParams& module_parameters);
};