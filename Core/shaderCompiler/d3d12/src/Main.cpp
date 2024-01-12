#include <iostream>

#include <atlbase.h>
#include <dxcapi.h>
#include <d3d12shader.h>

#define LOG(message) std::cout << message << std::endl

#define EXIT_ON_FAIL_HR(hresult, error_message)			\
		if (FAILED(hresult)) {							\
			std::cout << error_message << std::endl;	\
			std::exit(-1);								\
		}

#define EXIT_ON_FAIL(result, error_message)				\
		if (!result) {									\
			std::cout << error_message << std::endl;	\
			std::exit(-1);								\
		}

int main()
{
	LOG("---------------- Starting RabBit's D3D12 shader compiler ----------------");
	LOG("Shader files location: " << RB_SHADER_SOURCE);
	LOG("");

	// Check if the DLL's are loaded
	HANDLE dxc_handle = GetModuleHandle("dxcompiler.dll");
	HANDLE dxil_handle = GetModuleHandle("dxil.dll");
	EXIT_ON_FAIL(dxc_handle, "dxcompiler.dll was not loaded");
	EXIT_ON_FAIL(dxil_handle, "dxil.dll was not loaded");

	// Create compiler and utils

	CComPtr<IDxcUtils> utils;
	CComPtr<IDxcCompiler3> compiler;

	HRESULT hr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
	EXIT_ON_FAIL_HR(hr, "Failed to create utils instance");
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	EXIT_ON_FAIL_HR(hr, "Failed to create compiler instance");

	// Create default include handler (TODO Create our own)
	CComPtr<IDxcIncludeHandler> include_handler;
	utils->CreateDefaultIncludeHandler(&include_handler);

	
}