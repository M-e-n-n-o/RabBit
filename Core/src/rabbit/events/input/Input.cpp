#include "RabBitCommon.h"
#include "Input.h"

#include "events/KeyEvent.h"
#include "events/MouseEvent.h"

namespace RB::Events
{
    InputLayer* InputLayer::s_Instance = nullptr;

    InputLayer::InputLayer()
        : ApplicationLayer("InputLayer")
    {
        RB_ASSERT(LOGTAG_EVENT, s_Instance == nullptr, "Can only have one InputLayer class!");
        s_Instance = this;
    }

    bool InputLayer::IsKeyDown(const KeyCode& key)
    {
        InputLayer* l = GetInstance();
        return l->m_KeyMap[key];
    }

    bool InputLayer::IsMouseKeyDown(const MouseCode& mouse_button)
    {
        InputLayer* l = GetInstance();
        return l->m_MouseMap[mouse_button];
    }

    Math::Float2 InputLayer::GetMousePos()
    {
        InputLayer* l = GetInstance();
        return l->m_MousePos;
    }

    bool InputLayer::OnEvent(Event& event)
    {
        BindEvent<KeyPressedEvent>([&](KeyPressedEvent& e)
            {
                m_KeyMap[e.GetKeyCode()] = true;
            }, event);

        BindEvent<KeyReleasedEvent>([&](KeyReleasedEvent& e)
            {
                m_KeyMap[e.GetKeyCode()] = false;
            }, event);

        BindEvent<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
            {
                m_MouseMap[e.GetMouseButton()] = true;
            }, event);

        BindEvent<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent& e)
            {
                m_MouseMap[e.GetMouseButton()] = false;
            }, event);

        BindEvent<MouseMovedEvent>([&](MouseMovedEvent& e)
            {
                m_MousePos.x = e.GetMouseX();
                m_MousePos.y = e.GetMouseY();
            }, event);

        // Still push all these inputs to other layers
        return false;
    }
}