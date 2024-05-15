#include "RabBitCommon.h"
#include "Window.h"
#include "graphics/Renderer.h"
#include "graphics/d3d12/window/WindowD3D12.h"
#include "input/events/WindowEvent.h"

using namespace RB::Input::Events;

namespace RB::Graphics
{
	Window::Window()
		: m_AskedForClose(false)
		, m_InFocus(true)
	{
	}

	Window::~Window()
	{
		// Destroy window in implementation class!
	}

	bool Window::InFocus() const
	{
		return m_InFocus;
	}

	bool Window::ShouldClose() const
	{
		return m_AskedForClose;
	}
	
	void Window::ProcessEvent(WindowEvent& window_event)
	{
		// Only handle window events of this window
		if (!IsSameWindow(window_event.GetWindowHandle()))
		{
			return;
		}

		switch (window_event.GetEventType())
		{
		case EventType::WindowResize:
		{
			WindowResizeEvent* resize_event = static_cast<WindowResizeEvent*>(&window_event);

			uint32_t width = resize_event->GetWidth();
			uint32_t height = resize_event->GetHeight();

			OnResize(width, height);

			window_event.SetProcessed(true);
		}
		break;

		case EventType::WindowFocus:
		{
			m_InFocus = true;

			window_event.SetProcessed(true);
		}
		break;

		case EventType::WindowLostFocus:
		{
			m_InFocus = false;

			window_event.SetProcessed(true);
		}
		break;

		case EventType::WindowCloseRequest:
		{
			m_AskedForClose = true;

			window_event.SetProcessed(true);
		}
		break;

		case EventType::WindowClose:
		{
			DestroyWindow();

			window_event.SetProcessed(true);
		}
		break;

		default:
			break;
		}
	}
	
	Window* Window::Create(const char* window_name, uint32_t window_width, uint32_t window_height, uint32_t window_style)
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
		{
			D3D12::WindowArgs args = {};
			args.className		= L"RabBit WindowClass";
			args.instance		= GetModuleHandle(nullptr);
			args.width			= window_width;
			args.height			= window_height;
			args.windowStyle	= window_style;
			args.windowName		= window_name;

			return new D3D12::WindowD3D12(args);
		}
		default:
			RB_LOG_CRITICAL(LOGTAG_WINDOWING, "Did not yet implement the render interface for the set graphics API");
			break;
		}

		return nullptr;
	}
}