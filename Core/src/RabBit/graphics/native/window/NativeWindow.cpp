#include "RabBitCommon.h"
#include "NativeWindow.h"

#include "input/events/ApplicationEvent.h"
#include "input/events/MouseEvent.h"
#include "input/events/KeyEvent.h"

using namespace RB::Input;
using namespace RB::Input::Events;

namespace RB::Graphics::Native::Window
{
	// Window callback function
	LRESULT CALLBACK WindowCallback(HWND, UINT, WPARAM, LPARAM);

	std::unordered_map<HWND, EventListener*> WindowListeners;


	NativeWindow::NativeWindow(const WindowArgs args, EventListener* listener)
		: m_WindowHandle(nullptr)
		, m_Listener(listener)
	{
		RegisterWindowCLass(args.instance, args.className);
		CreateWindow(args.instance, args.className, args.windowName, args.width, args.height, args.extendedStyle, args.style, args.borderless);

		WindowListeners.insert(std::pair<HWND, EventListener*>(m_WindowHandle, listener));
	}

	NativeWindow::~NativeWindow()
	{
		WindowListeners.erase(m_WindowHandle);

		DestroyWindow(m_WindowHandle);
	}

	void NativeWindow::ProcessEvents()
	{
		MSG message = {};
		if (PeekMessage(&message, m_WindowHandle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	void NativeWindow::RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name)
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
		RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, SUCCEEDED(result), "Failed to register window");
	}

	void NativeWindow::CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height, DWORD extendedStyle, DWORD style, bool borderless)
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

		m_WindowHandle = ::CreateWindowExW(
			extendedStyle,
			class_name,
			window_title,
			style,
			window_x,
			window_y,
			window_width,
			window_height,
			NULL,
			NULL,
			instance,
			nullptr
		);

		RB_ASSERT_FATAL_RELEASE(LOGTAG_GRAPHICS, m_WindowHandle, "Failed to create window");

		if (borderless)
		{
			SetWindowLong(m_WindowHandle, GWL_STYLE, NULL);
		}

		//long wAttr = GetWindowLong(m_WindowHandle, GWL_EXSTYLE);
		//SetWindowLong(m_WindowHandle, GWL_EXSTYLE, wAttr | WS_EX_LAYERED);
		//SetLayeredWindowAttributes(m_WindowHandle, 0, 0xFF / 2, 0x02);
	}

	void NativeWindow::ShowWindow()
	{
		::ShowWindow(m_WindowHandle, SW_SHOW);
	}

	LRESULT CALLBACK WindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		EventListener* listener = nullptr;

		auto got = WindowListeners.find(hwnd);
		if (got != WindowListeners.end())
		{
			listener = got->second;
		}

		switch (message)
		{
		case WM_PAINT:
		{
			Event& e = WindowRenderEvent();
			listener->OnEvent(e);
		}
		break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			Event& e = KeyPressedEvent(static_cast<KeyCode>(wParam), false);
			listener->OnEvent(e);
		}
		break;
		case WM_SYSCHAR:
			break;
		case WM_SIZE:
		{
			RECT client_rect = {};
			::GetClientRect(hwnd, &client_rect);

			uint32_t width = client_rect.right - client_rect.left;
			uint32_t height = client_rect.bottom - client_rect.top;

			Event& e = WindowResizeEvent(width, height);
			listener->OnEvent(e);
		}
		break;
		case WM_CLOSE:
		{
			Event& e = WindowCloseEvent();
			listener->OnEvent(e);
		}
		break;
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;
		default:
		{
			return DefWindowProcW(hwnd, message, wParam, lParam);
		}
		}

		return 0;
	}
}
