#pragma once

#include "RabBitCommon.h"
#include "input/events/Event.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

namespace RB::Graphics::Native::Window
{
	struct WindowArgs
	{
		HINSTANCE		instance;
		const wchar_t*	className;
		const wchar_t*	windowName;
		uint32_t		width;
		uint32_t		height;
	};

	class NativeWindow
	{
	public:
		NativeWindow(const WindowArgs args, Input::Events::EventListener* listener);
		~NativeWindow();

		void ProcessEvents();

		void ShowWindow();

		HWND GetHandle() const { return m_WindowHandle; }

	private:
		void RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name);

		void CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height);

		HWND m_WindowHandle;

		Input::Events::EventListener* m_Listener;
	};
}