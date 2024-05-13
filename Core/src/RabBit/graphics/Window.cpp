#include "RabBitCommon.h"
#include "Window.h"
#include "graphics/Renderer.h"
#include "graphics/d3d12/window/WindowD3D12.h"
#include "input/events/WindowEvent.h"

using namespace RB::Input::Events;

namespace RB::Graphics
{
	Window::Window()
		: EventListener(kEventCat_Window)
		, m_AskedForClose(false)
		, m_InFocus(true)
	{
	}

	Window::~Window()
	{
		DestroyWindow();
	}

	bool Window::InFocus() const
	{
		return m_InFocus;
	}

	bool Window::ShouldClose() const
	{
		return m_AskedForClose;
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
			DestroyWindow();
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