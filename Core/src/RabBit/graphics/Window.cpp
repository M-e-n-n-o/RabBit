#include "RabBitCommon.h"
#include "Window.h"
#include "graphics/native/GraphicsDevice.h"
#include "graphics/native/window/NativeWindow.h"
#include "graphics/native/window/SwapChain.h"
#include "graphics/native/DeviceEngine.h"
#include "input/events/ApplicationEvent.h"

using namespace RB::Input::Events;
using namespace RB::Graphics::Native;
using namespace RB::Graphics::Native::Window;

namespace RB::Graphics
{
	Window::Window(const char* window_name, Input::Events::EventListener* listener, uint32_t window_width, uint32_t window_height)
		: m_Minimized(window_width == 0 && window_height == 0)
		, m_Listener(listener)
		, m_IsValid(true)
	{
		WindowArgs args = {};
		args.windowName = CharToWchar(window_name);
		args.className	= L"RabBit WindowClass";
		args.instance	= GetModuleHandle(nullptr);
		args.width		= window_width;
		args.height		= window_height;

		m_NativeWindow = new NativeWindow(args, this);

		m_SwapChain = new SwapChain(g_GraphicsDevice->GetGraphicsEngine()->GetCommandQueue(), m_NativeWindow->GetHandle(), window_width, window_height, BACK_BUFFER_COUNT);

		m_NativeWindow->ShowWindow();
	}

	Window::~Window()
	{
		DestroyWindow();
	}

	void Window::Update()
	{
		m_NativeWindow->ProcessEvents();
	}

	void Window::Resize(uint32_t width, uint32_t height)
	{
		if (m_SwapChain->GetWidth() == width && m_SwapChain->GetHeight() == height)
		{
			return;
		}

		m_Minimized = (width == 0 && height == 0);

		width  = std::max(1u, width);
		height = std::max(1u, height);

		m_SwapChain->Resize(width, height);
	}

	uint32_t Window::GetWidth() const
	{
		return m_SwapChain->GetWidth();
	}

	uint32_t Window::GetHeight() const
	{
		return m_SwapChain->GetHeight();
	}

	void Window::DestroyWindow()
	{
		m_IsValid = false;

		delete m_SwapChain;
		delete m_NativeWindow;
	}
	
	void Window::OnEvent(Event& event)
	{
		switch (event.GetEventType())
		{
		case EventType::WindowResize:
		{
			WindowResizeEvent* resize_event = static_cast<WindowResizeEvent*>(&event);

			uint32_t width = resize_event->GetWidth();
			uint32_t height = resize_event->GetHeight();

			g_GraphicsDevice->GetGraphicsEngine()->WaitForIdle();

			Resize(width, height);
		}
		break;

		case EventType::WindowClose:
		{
			// Only close the window yourself when there are no listeners attached
			if (!m_Listener)
			{
				DestroyWindow();
			}
		}
		break;

		default:
			break;
		}

		if (m_Listener)
		{
			m_Listener->OnEvent(event);
		}
	}
}