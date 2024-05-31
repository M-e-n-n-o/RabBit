#include <filesystem>
#include <sstream>
#include <regex>
#include <cstring>

#include <atlbase.h>
#include <dxcapi.h>
#include <d3d12shader.h>

#include "Compiler.h"
#include "ShaderWriter.h"
#include "Utils.h"

void RetrieveFiles(const std::filesystem::path& path, std::vector<std::wstring>&files);

/*

	RabBit D3D12 Shader Compiler
	- It only compiles files with the ".hlsl" extension, so ".h" files serve as include files (a bridge between cpp and hlsl)
	- It writes all the compiled shaders into the generated folder which the engine can include

*/

int main(int argc, char* argv[])
{
	LOGW(L"---------------- Starting RabBit's D3D12 shader compiler ----------------");

	std::string shader_bin_dir;

	for (int i = 0; i < argc; i++)
	{
		if (std::strstr("-shadersBin", argv[i]))
		{
			if (i + 1 < argc)
			{
				shader_bin_dir = argv[i + 1];
			}
		}
	}

	std::string shader_files_dir = RB_SHADER_SOURCE;
	LOGW(L"Shader files directory: " << shader_files_dir.c_str());

	if (shader_bin_dir.empty())
	{
		LOGW(L"Did not find the shader bin directory from the launch arguments, using default");
		shader_bin_dir = RB_OUTPUT_FOLDER;
	}

	LOGW(L"Shader bin directory: " << shader_bin_dir.c_str());

	LOGW(L"");

	// Check if the DLL's are loaded
	HANDLE dxc_handle = GetModuleHandle("dxcompiler.dll");
	HANDLE dxil_handle = GetModuleHandle("dxil.dll");
	EXIT_ON_FAIL(dxc_handle, L"dxcompiler.dll was not loaded");
	EXIT_ON_FAIL(dxil_handle, L"dxil.dll was not loaded");

	std::vector<std::wstring> files;
	RetrieveFiles(shader_files_dir, files);

	Compiler compiler;
	compiler.CompileFiles(files);

	LOGW(L"");

	ShaderWriter writer;
	writer.WriteOutShaders(RB_GRAPHICS_FOLDER, shader_files_dir, shader_bin_dir, compiler.GetCompiledShaders());

	LOGW(L"");
	LOGW(L"-------------------------------------------------------------------------");
	LOGW(L"Succesfully finished compiling the shaders");
}

void RetrieveFiles(const std::filesystem::path& path, std::vector<std::wstring>& files)
{
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_directory())
		{
			RetrieveFiles(entry.path(), files);
		}
		else
		{
			if (std::wstring(entry.path().c_str()).find(L".hlsl") != std::wstring::npos)
			{
				files.push_back(entry.path().c_str());
			}
		}
	}
}