#pragma once
#include <atlbase.h>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <vector>
#include <string>

class Compiler
{
public:
	Compiler();

	void CompileFiles(std::vector<std::wstring>& files);

private:
	enum class ShaderStage : uint8_t
	{
		kUnkown = 0,
		kVertex,
		kPixel,
		kCompute
	};

	struct ShaderEntry
	{
		std::wstring entryName;
		ShaderStage	 stage;
	};

	void RetrieveShaderEntries(DxcBuffer& source, std::vector<ShaderEntry>& entries);
	void GetShaderStages(const char* source, ShaderStage stage, const char* stage_name, std::vector<ShaderEntry>& entries);

	CComPtr<IDxcUtils>			m_Utils;
	CComPtr<IDxcCompiler3>		m_Compiler;
	CComPtr<IDxcIncludeHandler> m_IncludeHandler;
};