#include <filesystem>
#include <sstream>
#include <regex>
#include <cstring>

#include "Compiler.h"
#include "ShaderWriter.h"
#include "Utils.h"

/*

	RabBit D3D12/VK Shader Compiler
	- It only compiles files with the ".slang" extension, so ".h" files serve as include files (a bridge between cpp and hlsl)
	- It writes all the compiled shaders into the generated folder which the engine can include

*/

int main(int argc, char* argv[])
{
#if RB_SHADER_COMPILER_D3D12
	LOGW(L"---------------- Starting RabBit's D3D12 shader compiler ----------------");
#elif RB_SHADER_COMPILER_VK
	LOGW(L"---------------- Starting RabBit's vulkan shader compiler ----------------");
#endif

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

	LOG("Shader files directory: " << RB_SHADER_SOURCE);

	if (shader_bin_dir.empty())
	{
		LOGW(L"Did not find the shader bin directory from the launch arguments, using default");
		shader_bin_dir = RB_OUTPUT_FOLDER;
	}

	LOGW(L"Shader bin directory: " << shader_bin_dir.c_str());

	LOGW(L"");

	Compiler compiler;
	compiler.CompileFiles(RB_SHADER_SOURCE);

	auto reflection = compiler.GetShaderReflection();
	auto blobs = compiler.GetShaderBlobs();

	LOGW(L"");

	ShaderWriter writer;
	writer.WriteOutShaders(RB_DEFINE_FOLDER, shader_bin_dir, reflection, blobs);

	LOGW(L"");
	LOGW(L"-------------------------------------------------------------------------");
	LOGW(L"Succesfully finished compiling the shaders");
}