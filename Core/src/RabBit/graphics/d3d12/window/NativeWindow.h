#pragma once

#include "RabBitCommon.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

namespace RB::Graphics::D3D12::Window
{
	struct WindowArgs
	{
		HINSTANCE		instance;
		wchar_t*		className;
		wchar_t*		windowName;
		uint32_t		width;
		uint32_t		height;
		DWORD			extendedStyle;
		DWORD			style;
		bool			borderless;
	};

	class NativeWindow
	{
	public:
		NativeWindow(const WindowArgs args);
		~NativeWindow();

		void ProcessEvents();

		void ShowWindow();

		HWND GetHandle() const { return m_WindowHandle; }

	private:
		void RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name);

		void CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height, DWORD extendedStyle, DWORD style, bool borderless);

		HWND m_WindowHandle;
	};
}