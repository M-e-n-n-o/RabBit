#include "RabBitCommon.h"
#include "Window.h"
#include "graphics/Renderer.h"
#include "graphics/Display.h"
#include "graphics/d3d12/window/WindowD3D12.h"
#include "input/events/WindowEvent.h"

using namespace RB::Input::Events;

namespace RB::Graphics
{
	Window::Window(bool is_fullscreen)
		: m_InFocus(true)
		, m_IsFullscreen(is_fullscreen)
		, m_OriginalRect(0, 0, -1, -1)
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

	float Window::GetAspectRatio() const
	{
		return (float) GetWidth() / (float) GetHeight();
	}

	float Window::GetVirtualAspectRatio() const
	{
		return (float) GetVirtualWidth() / (float) GetVirtualHeight();
	}

	void Window::ToggleFullscreen()
	{
		if (m_IsFullscreen)
		{
			SetBorderless(false);
			Resize(m_OriginalRect.x, m_OriginalRect.y, m_OriginalRect.z, m_OriginalRect.w);
		}
		else
		{
			m_OriginalRect = GetWindowRectangle();

			SetBorderless(true);

			Math::Float2 res = GetParentDisplay()->GetResolution();
			Resize(res.x, res.y, 0, 0);
		}

		m_IsFullscreen = !m_IsFullscreen;
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
		case EventType::WindowCreated:
		{
			RB_LOG_ERROR(LOGTAG_WINDOWING, "We should not receive this Window Created event");
		}
		break;

		case EventType::WindowMoved:
		{
			RB_LOG(LOGTAG_WINDOWING, "Moved");
		}
		break;

		case EventType::WindowFullscreenToggle:
		{
			ToggleFullscreen();
		}
		break;

		case EventType::WindowResize:
		{
			WindowResizeEvent* resize_event = static_cast<WindowResizeEvent*>(&window_event);

			uint32_t width = resize_event->GetWidth();
			uint32_t height = resize_event->GetHeight();

			RB_LOG(LOGTAG_WINDOWING, "Resizing window to (%d x %d)", width, height);

			if (resize_event->ShouldResizeSwapChain())
			{
				// Do the swapchain resize (window is already resized here)
				OnResize(width, height);
			}
			else
			{
				// First resize the actual window, which will create another resize event which will resize the swapchain
				Resize(width, height, -1, -1);
			}

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
			DestroyWindow();
		}
		break;

		case EventType::WindowClose:
		{
			RB_LOG_ERROR(LOGTAG_WINDOWING, "Window::ProcessEvent should not get a WindowClose event!");
		}
		break;

		default:
			break;
		}
	}
	
	Window* Window::Create(const char* window_name, Display* display, uint32_t window_style, uint32_t virtual_scale, uint32_t virtual_aspect)
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
		{


			D3D12::WindowArgs args = {};
			args.className		= L"RabBit WindowClass";
			args.instance		= GetModuleHandle(nullptr);
			args.fullscreen		= true;
			args.width			= display->GetResolution().x;
			args.height			= display->GetResolution().y;
			args.virtualScale	= virtual_scale;
			args.virtualAspect	= virtual_aspect;
			args.windowStyle	= window_style;
			args.windowName		= window_name;

			return new D3D12::WindowD3D12(args);
		}
		default:
			RB_LOG_CRITICAL(LOGTAG_WINDOWING, "Did not yet implement the window class for the set graphics API");
			break;
		}

		return nullptr;
	}

	Window* Window::Create(const char* window_name, uint32_t window_width, uint32_t window_height, uint32_t window_style, uint32_t virtual_scale, uint32_t virtual_aspect)
	{
		switch (Renderer::GetAPI())
		{
		case RenderAPI::D3D12:
		{
			D3D12::WindowArgs args = {};
			args.className		= L"RabBit WindowClass";
			args.instance		= GetModuleHandle(nullptr);
			args.fullscreen		= false;
			args.width			= window_width;
			args.height			= window_height;
			args.virtualScale	= virtual_scale;
			args.virtualAspect	= virtual_aspect;
			args.windowStyle	= window_style;
			args.windowName		= window_name;

			return new D3D12::WindowD3D12(args);
		}
		default:
			RB_LOG_CRITICAL(LOGTAG_WINDOWING, "Did not yet implement the window class for the set graphics API");
			break;
		}

		return nullptr;
	}
}