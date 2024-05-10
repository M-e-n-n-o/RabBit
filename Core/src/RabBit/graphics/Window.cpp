#include "RabBitCommon.h"
#include "Window.h"
#include "graphics/Renderer.h"
#include "graphics/d3d12/window/WindowD3D12.h"
#include "input/events/WindowEvent.h"

using namespace RB::Input::Events;
using namespace RB::Graphics::D3D12;

namespace RB::Graphics
{
	Window::Window(const char* window_name, uint32_t window_width, uint32_t window_height, uint32_t window_style)
		: EventListener(kEventCat_Window)
		, m_Minimized(window_width == 0 && window_height == 0)
		, m_IsValid(true)
		, m_InFocus(true)
		, m_AskedForClose(false)
	{
		WindowArgs args		= {};
		args.className		= L"RabBit WindowClass";
		args.instance		= GetModuleHandle(nullptr);
		args.width			= window_width;
		args.height			= window_height;
		args.extendedStyle	= NULL;
		args.style			= WS_OVERLAPPEDWINDOW;
		args.borderless		= false;
		//args.windowName		= (wchar_t*) ALLOC_STACK(sizeof(wchar_t) * strlen(window_name) + 1);
		args.windowName		= new wchar_t[strlen(window_name) + 1]; // <- !! TODO: THIS IS A SMALL, NOT SO IMPORTANT, MEMORY LEAK, BUT STILL NEED TO FIX THIS !!
		CharToWchar(window_name, args.windowName);

		if (window_style & kWindowStyle_Borderless)
		{
			args.borderless = true;
		}

		if (window_style & kWindowStyle_SemiTransparent)
		{
			args.extendedStyle = WS_EX_NOREDIRECTIONBITMAP;
		}

		m_NativeWindow = new NativeWindow(args);

		m_SwapChain = new SwapChain(
			g_GraphicsDevice->GetFactory(),
			g_GraphicsDevice->GetGraphicsQueue()->GetCommandQueue(), 
			m_NativeWindow->GetHandle(), 
			window_width, window_height, 
			BACK_BUFFER_COUNT, 
			DXGI_FORMAT_R8G8B8A8_UNORM, 
			(bool) (window_style & kWindowStyle_SemiTransparent)
		);

		m_NativeWindow->ShowWindow();
	}

	Window::~Window()
	{
		DestroyWindow();
	}

	bool Window::IsSameWindow(void* window_handle) const
	{
		return window_handle == m_NativeWindow->GetHandle();
	}

	void Window::DestroyWindow()
	{
		g_GraphicsDevice->WaitUntilIdle();

		m_IsValid = false;

		delete m_SwapChain;
		delete m_NativeWindow;
	}
	
	void Window::OnEvent(Event& event)
	{
		WindowEvent* window_event = static_cast<WindowEvent*>(&event);

		// Only handle window events of this window
		if (!IsSameWindow(window_event->GetWindowHandle()))
		{
			return;
		}

		switch (event.GetEventType())
		{
		case EventType::WindowResize:
		{
			WindowResizeEvent* resize_event = static_cast<WindowResizeEvent*>(&event);

			uint32_t width = resize_event->GetWidth();
			uint32_t height = resize_event->GetHeight();

			OnResize(width, height);
		}
		break;

		case EventType::WindowFocus:
		{
			m_InFocus = true;
		}
		break;

		case EventType::WindowLostFocus:
		{
			m_InFocus = false;
		}
		break;

		case EventType::WindowCloseRequest:
		{
			m_AskedForClose = true;
		}
		break;

		case EventType::WindowClose:
		{
			m_IsValid = false;
			DestroyWindow();
		}
		break;

		default:
			break;
		}
	}
	
	Window* Window::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
			return new D3D12::WindowD3D12();
		default:
			RB_LOG_CRITICAL(LOGTAG_GRAPHICS, "Did not yet implement the render interface for the set graphics API");
			break;
		}

		return nullptr;
	}
}