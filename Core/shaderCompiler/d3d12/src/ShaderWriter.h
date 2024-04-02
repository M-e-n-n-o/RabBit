#pragma once
#include "Shader.h"

class ShaderWriter
{
public:
	ShaderWriter();

	void WriteOutShaders(const std::string& defines_folder, const std::string& bin_folder, const std::vector<Shader>& shaders);
};