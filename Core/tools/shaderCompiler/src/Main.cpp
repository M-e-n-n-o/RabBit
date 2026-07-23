#include <filesystem>
#include <sstream>
#include <regex>
#include <cstring>

#include "Compiler.h"
#include "ShaderWriter.h"
#include "Log.h"
#include "ArgParsing.h"

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

	DEFINE_FIND_LAUNCH_ARG(argc, argv);

	const char* extra_source_dir = FindLaunchArg("-extraSrc");
	const char* shader_bin_dir = FindLaunchArg("-shadersBin");

	std::vector<std::string> source_dirs;
	source_dirs.push_back(RB_SHADER_SOURCE);
	if (extra_source_dir != nullptr)
		source_dirs.push_back(extra_source_dir);

	LOG("Shader files directory: ");
	for (const auto& path : source_dirs)
		LOG("\t" << path);

	if (shader_bin_dir == nullptr)
	{
		LOGW(L"Did not find the shader bin directory from the launch arguments, using default");
		shader_bin_dir = RB_OUTPUT_FOLDER;
	}

	LOGW(L"Shader bin directory: " << shader_bin_dir);

	LOGW(L"");

	Compiler compiler;
	compiler.CompileFiles(source_dirs);

	auto reflection = compiler.GetShaderReflection();
	auto blobs = compiler.GetShaderBlobs();
	auto params = compiler.GetModuleParameters();

	LOGW(L"");

	ShaderWriter writer;
	writer.WriteOutShaders(RB_DEFINE_FOLDER, shader_bin_dir, reflection, blobs, params);

	LOGW(L"");
	LOGW(L"-------------------------------------------------------------------------");
	LOGW(L"Succesfully finished compiling the shaders");

	return 0;
}