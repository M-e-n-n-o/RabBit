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
		int GetCategoryFlags() const override { return kEventCat_Keyboard; }

	private:
		KeyCode m_KeyCode;
	};


	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(const KeyCode keyCode, bool isRepeat) : KeyEvent(keyCode), m_IsRepeat(isRepeat) {}

		bool IsRepeatEvent() const { return m_IsRepeat; }

		DEFINE_CLASS_TYPE(KeyPressedEvent, KeyPressed, false)

	private:
		bool m_IsRepeat;
	};


	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(const KeyCode keyCode) : KeyEvent(keyCode) {}

		DEFINE_CLASS_TYPE(KeyReleasedEvent, KeyReleased, false)
	};


	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(const KeyCode keyCode) : KeyEvent(keyCode) {}

		DEFINE_CLASS_TYPE(KeyTypedEvent, KeyTyped, false)
	};
}
