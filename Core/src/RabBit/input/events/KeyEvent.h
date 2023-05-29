#pragma once

#include "Event.h"
#include "input/KeyCodes.h"

namespace RB::Input::Events
{
	class KeyEvent : public Event
	{
	protected:
		KeyEvent(const KeyCode keyCode) : m_KeyCode(keyCode) {}
		virtual ~KeyEvent() = default;

	public:
		KeyCode GetKeyCode() const { return m_KeyCode; }
		int GetCategoryFlags() const override { return EventCatInput | EventCatKeyboard; }

	private:
		KeyCode m_KeyCode;
	};


	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(const KeyCode keyCode, bool isRepeat) : KeyEvent(keyCode), m_IsRepeat(isRepeat) {}

		bool IsRepeatEvent() const { return m_IsRepeat; }

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool m_IsRepeat;
	};


	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(const KeyCode keyCode) : KeyEvent(keyCode) {}

		EVENT_CLASS_TYPE(KeyReleased)
	};


	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(const KeyCode keyCode) : KeyEvent(keyCode) {}

		EVENT_CLASS_TYPE(KeyTyped)
	};
}
