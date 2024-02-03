#include <filesystem>
#include <sstream>
#include <regex>

#include <atlbase.h>
#include <dxcapi.h>
#include <d3d12shader.h>

#include "Compiler.h"
#include "Utils.h"

void RetrieveFiles(const std::filesystem::path& path, std::vector<std::wstring>&files);

/*

	RabBit D3D12 Shader Compiler
	- It only compiles files with the ".hlsl" extension, so ".h" files serve as include files (a bridge between cpp and hlsl)
	- It writes all the compiled shaders into the generated folder which the engine can include

*/

int main()
{
	LOG(L"---------------- Starting RabBit's D3D12 shader compiler ----------------");
	LOG(L"Shader files location: " << RB_SHADER_SOURCE);
	LOG(L"");

	// Check if the DLL's are loaded
	HANDLE dxc_handle = GetModuleHandle("dxcompiler.dll");
	HANDLE dxil_handle = GetModuleHandle("dxil.dll");
	EXIT_ON_FAIL(dxc_handle, L"dxcompiler.dll was not loaded");
	EXIT_ON_FAIL(dxil_handle, L"dxil.dll was not loaded");

	std::vector<std::wstring> files;
	RetrieveFiles(RB_SHADER_SOURCE, files);

	Compiler compiler;
	compiler.CompileFiles(files);

	LOG(L"");
	LOG(L"-------------------------------------------------------------------------");
	LOG(L"Succesfully finished compiling the shaders");
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