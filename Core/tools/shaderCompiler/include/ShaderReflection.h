#pragma once

#include <vector>
#include <string>

namespace RB::ShaderCompiler
{
	enum class Stage : uint32_t
	{
		kVertex = 0,
		kPixel,
		kCompute
	};

	enum class ParamType : uint32_t
	{
		kConstantBuffer = 0,
		kSampler
	};

	enum class ScalarType : uint32_t
	{
		Float32 = 0,
		Float16,
		UInt32,
		UInt16,
		Int32,
		Int16,
	};

	struct GlobalParameter
	{
		std::string name;
		ParamType	type;
		uint32_t	bindingIndex;
		uint32_t	size;
	};

	struct EntryParameter
	{
		std::string name;
		uint32_t	bindingOffset;
		uint32_t	size;
	};

	struct VertexEntryParameter
	{
		std::string name;
		std::string semantic;
		uint32_t	semanticIndex;
		uint32_t	elements; // e.g. 3 for float3
		ScalarType	scalarType;
		uint32_t	size;
	};

	struct Blob
	{
		uint64_t offsetInFile;
		uint64_t size;
	};

	struct ShaderReflection
	{
		std::string							entryName;
		Stage								stage;
		Blob								shaderBlob;
		std::vector<GlobalParameter>		globalParameters;
		std::vector<EntryParameter>			entryPointParameters; // Push constants
		std::vector<VertexEntryParameter>	vertexParameters;
	};
}