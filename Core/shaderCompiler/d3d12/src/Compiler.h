#pragma once
#include <atlbase.h>
#include <dxcapi.h>
#include <d3d12shader.h>
#include "Shader.h"

class Compiler
{
public:
	Compiler();

	void CompileFiles(std::vector<std::wstring>& files);

	std::vector<Shader> GetCompiledShaders() const { return m_CompiledShaders; }

private:
	void RetrieveShaderEntries(DxcBuffer& source, std::vector<Shader>& entries);
	void GetShaderStages(const char* source, ShaderStage stage, const char* prefix, std::vector<Shader>& entries);

	CComPtr<IDxcUtils>			m_Utils;
	CComPtr<IDxcCompiler3>		m_Compiler;
	CComPtr<IDxcIncludeHandler> m_IncludeHandler;

	std::vector<Shader>			m_CompiledShaders;
};