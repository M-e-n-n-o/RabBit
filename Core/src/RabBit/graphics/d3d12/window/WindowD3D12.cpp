#include "WindowD3D12.h"
#include "WindowD3D12.h"
#include "WindowD3D12.h"
#include "WindowD3D12.h"
#include "WindowD3D12.h"
#include "RabBitCommon.h"
#include "WindowD3D12.h"

#include "input/events/WindowEvent.h"
#include "input/events/MouseEvent.h"
#include "input/events/KeyEvent.h"

using namespace RB::Input;
using namespace RB::Input::Events;

namespace RB::Graphics::D3D12
{
	// Window callback function
	LRESULT CALLBACK WindowCallback(HWND, UINT, WPARAM, LPARAM);

	WindowD3D12::WindowD3D12(const WindowArgs args)
		: m_WindowHandle(nullptr)
	{
		RegisterWindowCLass(args.instance, args.className);
		CreateWindow(args.instance, args.className, args.windowName, args.width, args.height, args.extendedStyle, args.style, args.borderless);
	}

	WindowD3D12::~WindowD3D12()
	{
		DestroyWindow(m_WindowHandle);
	}

	void WindowD3D12::Update()
	{
		MSG message = {};
		if (PeekMessage(&message, m_WindowHandle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	uint32_t WindowD3D12::GetWidth() const
	{
		return m_SwapChain->GetWidth();
	}

	uint32_t WindowD3D12::GetHeight() const
	{
		return m_SwapChain->GetHeight();
	}

	void WindowD3D12::Resize(uint32_t width, uint32_t height, int32_t x, int32_t y)
	{
		// TODO: MAKE SURE THAT -1 MAKES SURE THAT THE WINDOW IS CENTERED TO THE MONITOR
		static_assert(false);

		//SetWindowPos(m_NativeWindow->GetHandle(), HWND_TOP, x, y, width, height, NULL);
	}

	Graphics::Texture2D* WindowD3D12::GetCurrentBackBuffer()
	{
		return nullptr;
	}

	void WindowD3D12::OnResize(uint32_t width, uint32_t height)
	{
		if (m_SwapChain->GetWidth() == width && m_SwapChain->GetHeight() == height)
		{
			return;
		}

		g_GraphicsDevice->WaitUntilIdle();

		m_Minimized = (width == 0 && height == 0);

		width = std::max(1u, width);
		height = std::max(1u, height);

		m_SwapChain->Resize(width, height);
	}

	void WindowD3D12::DestroyWindow()
	{
		g_GraphicsDevice->WaitUntilIdle();

		delete m_SwapChain;
	}

	void WindowD3D12::RegisterWindowCLass(HINSTANCE instance, const wchar_t* class_name)
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

	void WindowD3D12::CreateWindow(HINSTANCE instance, const wchar_t* class_name, const wchar_t* window_title, uint32_t width, uint32_t height, DWORD extendedStyle, DWORD style, bool borderless)
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

	void WindowD3D12::ShowWindow()
	{
		::ShowWindow(m_WindowHandle, SW_SHOW);
	}

	LRESULT CALLBACK WindowCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			Event* e = new WindowRenderEvent(hwnd);
			g_EventManager->InsertEvent(e);
		}
		break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			Event* e = new KeyPressedEvent(static_cast<KeyCode>(wParam), false);
			g_EventManager->InsertEvent(e);
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

			Event* e = new WindowResizeEvent(hwnd, width, height);
			g_EventManager->InsertEvent(e);
		}
		break;
		case WM_SETFOCUS:
		{
			Event* e = new WindowOnFocusEvent(hwnd);
			g_EventManager->InsertEvent(e);
		}
		break;
		case WM_KILLFOCUS:
		{
			Event* e = new WindowLostFocusEvent(hwnd);
			g_EventManager->InsertEvent(e);
		}
		break;
		case WM_CLOSE:
		{
			Event* e = new WindowCloseRequestEvent(hwnd);
			g_EventManager->InsertEvent(e);
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