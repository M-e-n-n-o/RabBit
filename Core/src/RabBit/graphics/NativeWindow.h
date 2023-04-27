#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

namespace RB::Graphics::NativeWindow
{
	extern HWND g_NativeWindowHandle;

	void RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name);

	void CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height);

	void ShowWindow();
}