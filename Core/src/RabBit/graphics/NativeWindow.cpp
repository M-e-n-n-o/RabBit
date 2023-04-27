#include "RabBitPch.h"
#include "NativeWindow.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>

namespace RB::Graphics::NativeWindow
{
	HWND g_NativeWindowHandle = nullptr;

	// Window callback function
	LRESULT CALLBACK WindowCallback(HWND, UINT, WPARAM, LPARAM);


	void RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name)
	{
		WNDCLASSEXW window_class = {};

		window_class.cbSize			= sizeof(WNDCLASSEXW);
		window_class.style			= CS_HREDRAW | CS_VREDRAW;
		window_class.lpfnWndProc	= &WindowCallback;
		window_class.cbClsExtra		= 0;
		window_class.cbWndExtra		= 0;
		window_class.hInstance		= instance;
		window_class.hIcon			= ::LoadIcon(instance, "0");			// MAKEINTRESOURCE(APP_ICON)
		window_class.hCursor		= ::LoadCursor(NULL, IDC_ARROW);
		window_class.hbrBackground	= (HBRUSH) (COLOR_WINDOW + 1);
		window_class.lpszMenuName	= NULL;
		window_class.lpszClassName	= class_name;
		window_class.hIconSm		= ::LoadIcon(instance, "0");			// MAKEINTRESOURCE(APP_ICON)

		HRESULT result = ::RegisterClassExW(&window_class);
		RB_ASSERT_FATAL_RELEASE(SUCCEEDED(result), "Failed to register window");
	}

	void CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height)
	{
		int screen_width	= ::GetSystemMetrics(SM_CXSCREEN);
		int screen_height	= ::GetSystemMetrics(SM_CYSCREEN);

		RECT window_rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		::AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

		int window_width	= window_rect.right - window_rect.left;
		int window_height	= window_rect.bottom - window_rect.top;

		// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
		int window_x		= std::max<int>(0, (screen_width - window_width) / 2);
		int window_y		= std::max<int>(0, (screen_height - window_height) / 2);

		g_NativeWindowHandle = ::CreateWindowExW(
			NULL,
			class_name,
			window_title,
			WS_OVERLAPPEDWINDOW,
			window_x,
			window_y,
			window_width,
			window_height,
			NULL,
			NULL,
			instance,
			nullptr
		);

		RB_ASSERT_FATAL_RELEASE(g_NativeWindowHandle, "Failed to create window");
	}

	void ShowWindow()
	{
		::ShowWindow(g_NativeWindowHandle, SW_SHOW);
	}

	LRESULT CALLBACK WindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
		return 0;
	}
}
