#pragma once

#include <vector>
#include <functional>
#include <filesystem>

#include <slang.h>
#include <slang-com-ptr.h>
#include "../include/ShaderReflection.h"

class Compiler
{
public:
	Compiler();

	void CompileFiles(const char* base_path);

	std::vector<RB::ShaderCompiler::CompiledShader> GetShaderReflection() const { return m_Reflections; }
	std::vector<Slang::ComPtr<slang::IBlob>> GetShaderBlobs() const { return m_ShaderBlobs; }

private:
	void FindFiles(const char* base_path, std::vector<std::string>& dirs, std::vector<std::filesystem::path>& files);
	void ReflectEntryPointParameters(slang::EntryPointReflection* entry, RB::ShaderCompiler::CompiledShader* out_shader);
	void ReflectGlobalScope(slang::VariableLayoutReflection* layout, RB::ShaderCompiler::CompiledShader* out_shader);

	Slang::ComPtr<slang::IGlobalSession>			m_GlobalSession;
	std::vector<RB::ShaderCompiler::CompiledShader>	m_Reflections;
	std::vector< Slang::ComPtr<slang::IBlob>>		m_ShaderBlobs;
};