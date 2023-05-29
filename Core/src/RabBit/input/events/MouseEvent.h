#pragma once

#include "Event.h"
#include "input/MouseCodes.h"

namespace RB::Input::Events
{
	class MouseEvent : public Event
	{
	public:
		virtual EventType GetEventType() const override = 0;
		virtual const char* GetName() const override = 0;
		virtual int GetCategoryFlags() const override = 0;
	};

	class MouseMovedEvent : public MouseEvent
	{
	public:
		MouseMovedEvent(const float x, const float y) : m_MouseX(x), m_MouseY(y) {}

		float GetMouseX() const { return m_MouseX; }
		float GetMouseY() const { return m_MouseY; }

		EVENT_CLASS_TYPE(MouseMoved)
		int GetCategoryFlags() const override { return EventCatMouse | EventCatInput; }

	private:
		float m_MouseX;
		float m_MouseY;
	};


	class MouseScrolledEvent : public MouseEvent
	{
	public:
		MouseScrolledEvent(const float x, const float y) : m_OffsetX(x), m_OffsetY(y) {}

		float GetOffsetX() const { return m_OffsetX; }
		float GetOffsetY() const { return m_OffsetY; }

		EVENT_CLASS_TYPE(MouseScrolled)
		int GetCategoryFlags() const override { return EventCatMouse | EventCatInput; }

	private:
		float m_OffsetX;
		float m_OffsetY;
	};


	class MouseButtonEvent : public MouseEvent
	{
	protected:
		MouseButtonEvent(const MouseCode mouseCode) : m_MouseCode(mouseCode) {}
		virtual ~MouseButtonEvent() = default;

	public:
		MouseCode GetMouseButton() const { return m_MouseCode; }

		int GetCategoryFlags() const override { return EventCatMouse | EventCatInput | EventCatMouseButton; }

	protected:
		MouseCode m_MouseCode;
	};


	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(const MouseCode mouseCode) : MouseButtonEvent(mouseCode) {}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};


	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(const MouseCode mouseCode) : MouseButtonEvent(mouseCode) {}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}
