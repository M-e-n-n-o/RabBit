#pragma once

#include "utils/Ptr.h"
#include "input/events/Event.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

namespace RB::Graphics::Native::Window
{
	class NativeWindow
	{
	public:
		NativeWindow(Input::Events::EventListener* listener);
		~NativeWindow();

		void ProcessEvents();

		void RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name);

		void CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height);

		void ShowWindow();

		HWND GetWindowHandle() const { return m_NativeWindowHandle; }

	private:
		HWND m_NativeWindowHandle;

		Input::Events::EventListener* m_Listener;

		friend LRESULT CALLBACK WindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	};

	extern NativeWindow* g_NativeWindow;
}