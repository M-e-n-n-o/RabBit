#pragma once

#include "Event.h"
#include "input/KeyCodes.h"

namespace RB::Input::Events
{
    class KeyEvent : public Event
    {
    protected:
        KeyEvent(const KeyCode key_code) : m_KeyCode(key_code) {}
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
        KeyPressedEvent(const KeyCode key_code, bool is_repeat) : KeyEvent(key_code), m_IsRepeat(is_repeat) {}

        bool IsRepeatEvent() const { return m_IsRepeat; }

        DEFINE_CLASS_TYPE(KeyPressedEvent, KeyPressed, false)

    private:
        bool m_IsRepeat;
    };


    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(const KeyCode key_code) : KeyEvent(key_code) {}

        DEFINE_CLASS_TYPE(KeyReleasedEvent, KeyReleased, false)
    };


    class KeyTypedEvent : public KeyEvent
    {
    public:
        KeyTypedEvent(const KeyCode key_code) : KeyEvent(key_code) {}

        DEFINE_CLASS_TYPE(KeyTypedEvent, KeyTyped, false)
    };
}
