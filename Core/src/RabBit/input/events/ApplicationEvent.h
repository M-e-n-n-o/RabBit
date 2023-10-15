#pragma once

#include "Event.h"

namespace RB::Input::Events
{
	class WindowCreatedEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(WindowCreated)

		int GetCategoryFlags() const override { return kEventCat_Application; }
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(uint32_t width, uint32_t height) : m_Width(width), m_Height(height) {}

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		EVENT_CLASS_TYPE(WindowResize)

		int GetCategoryFlags() const override { return kEventCat_Application; }

	private:
		uint32_t m_Width;
		uint32_t m_Height;
	};

	class WindowCloseEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(WindowClose)

		int GetCategoryFlags() const override { return kEventCat_Application; }
	};

	class WindowRenderEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(WindowRender)

		int GetCategoryFlags() const override { return kEventCat_Application; }
	};

	class WindowOnFocusEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(WindowFocus)

		int GetCategoryFlags() const override { return kEventCat_Application; }
	};

	class WindowLostFocusEvent : public Event
	{
	public:
		EVENT_CLASS_TYPE(WindowLostFocus)

		int GetCategoryFlags() const override { return kEventCat_Application; }
	};
}