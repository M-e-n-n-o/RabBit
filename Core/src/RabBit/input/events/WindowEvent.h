#pragma once

#include "Event.h"

namespace RB::Input::Events
{
	class WindowEvent : public Event
	{
	public:
		WindowEvent(void* window_handle): m_WindowHandle(window_handle) {}
		virtual ~WindowEvent() = default;

		void* GetWindowHandle() const { return m_WindowHandle; }

		virtual EventType GetEventType() const override = 0;
		virtual const char* GetName() const override = 0;

		int GetCategoryFlags() const override { return kEventCat_Window; }

	private:
		void* m_WindowHandle;
	};

	class WindowCreatedEvent : public WindowEvent
	{
	public:
		WindowCreatedEvent(void* window_handle): WindowEvent(window_handle) {}

		EVENT_CLASS_TYPE(WindowCreated)
	};

	class WindowResizeEvent : public WindowEvent
	{
	public:
		WindowResizeEvent(void* window_handle, uint32_t width, uint32_t height) : WindowEvent(window_handle), m_Width(width), m_Height(height) {}

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		EVENT_CLASS_TYPE(WindowResize)

	private:
		uint32_t m_Width;
		uint32_t m_Height;
	};

	class WindowCloseEvent : public WindowEvent
	{
	public:
		WindowCloseEvent(void* window_handle) : WindowEvent(window_handle) {}

		EVENT_CLASS_TYPE(WindowClose)
	};

	class WindowCloseRequestEvent : public WindowEvent
	{
	public:
		WindowCloseRequestEvent(void* window_handle) : WindowEvent(window_handle) {}

		EVENT_CLASS_TYPE(WindowCloseRequest)
	};

	class WindowRenderEvent : public WindowEvent
	{
	public:
		WindowRenderEvent(void* window_handle) : WindowEvent(window_handle) {}

		EVENT_CLASS_TYPE(WindowRender)
	};

	class WindowOnFocusEvent : public WindowEvent
	{
	public:
		WindowOnFocusEvent(void* window_handle) : WindowEvent(window_handle) {}

		EVENT_CLASS_TYPE(WindowFocus)
	};

	class WindowLostFocusEvent : public WindowEvent
	{
	public:
		WindowLostFocusEvent(void* window_handle) : WindowEvent(window_handle) {}

		EVENT_CLASS_TYPE(WindowLostFocus)
	};
}