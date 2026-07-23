#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include <slang.h>
#include <slang-com-ptr.h>
#include "../include/ShaderReflection.h"

typedef std::unordered_map<std::string, std::vector<RB::ShaderCompiler::GlobalParameter>> ModuleParams;

class Compiler
{
public:
	Compiler();

	void CompileFiles(std::vector<std::string> source_dirs);

	const std::vector<RB::ShaderCompiler::ShaderReflection>& GetShaderReflection() const { return m_Reflections; }
	const std::vector<Slang::ComPtr<slang::IBlob>>& GetShaderBlobs() const { return m_ShaderBlobs; }
	const ModuleParams& GetModuleParameters() const { return m_ModuleParameters; }

private:
	void FindFiles(std::vector<std::string> source_dirs, std::vector<std::string>& dirs, std::vector<std::filesystem::path>& files);
	void ReflectEntryPointParameters(slang::EntryPointReflection* entry, RB::ShaderCompiler::ShaderReflection* out_shader);
	void ReflectGlobalScope(slang::VariableLayoutReflection* layout, std::vector<RB::ShaderCompiler::GlobalParameter>* out_parameters);

	Slang::ComPtr<slang::IGlobalSession>				m_GlobalSession;
	std::vector<RB::ShaderCompiler::ShaderReflection>	m_Reflections;
	std::vector<Slang::ComPtr<slang::IBlob>>			m_ShaderBlobs;
	ModuleParams										m_ModuleParameters;
};