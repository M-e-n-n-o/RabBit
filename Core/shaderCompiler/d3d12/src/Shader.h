#pragma once
#include <atlbase.h>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <vector>
#include <string>

enum class ShaderStage : uint8_t
{
	kUnkown = 0,
	kVertex,
	kPixel,
	kCompute
};

struct Shader
{
	std::wstring			entryName;
	ShaderStage				stage;
	CComPtr<IDxcBlob>		shaderBlob;
	CComPtr<IDxcBlob>		reflection;
};