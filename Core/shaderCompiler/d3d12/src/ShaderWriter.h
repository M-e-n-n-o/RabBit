#pragma once
#include "Shader.h"

class ShaderWriter
{
public:
	ShaderWriter();

	void WriteOutShaders(const std::wstring& defines_folder, const std::wstring& bin_folder, const std::vector<Shader>& shaders);
};