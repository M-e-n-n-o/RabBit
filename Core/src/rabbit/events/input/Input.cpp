#include "RabBitCommon.h"
#include "Input.h"

#include "events/KeyEvent.h"
#include "events/MouseEvent.h"

namespace RB::Events
{
    InputLayer* InputLayer::s_Instance = nullptr;

    InputLayer::InputLayer()
        : ApplicationLayer("InputLayer")
        , m_MousePosUpdated(false)
        , m_MouseScrollUpdated(false)
        , m_MouseScrollDelta(0.0f)
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

    Math::Float2 InputLayer::GetMousePosDelta()
    {
        InputLayer* l = GetInstance();
        return l->m_MousePos - l->m_PrevMousePos;
    }

    float InputLayer::GetMouseScrollDelta()
    {
        InputLayer* l = GetInstance();
        return l->m_MouseScrollDelta;
    }

    void InputLayer::OnUpdate(float delta_time)
    {
        if (m_MousePosUpdated)
        {
            m_MousePosUpdated = false;
        }
        else
        {
            m_PrevMousePos = m_MousePos;
        }

        if (m_MouseScrollUpdated)
        {
            m_MouseScrollUpdated = false;
        }
        else
        {
            m_MouseScrollDelta = 0.0f;
        }
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

        BindEvent<MouseScrolledEvent>([&](MouseScrolledEvent& e)
            {
                m_MouseScrollDelta = e.GetDeltaY();
                m_MouseScrollUpdated = true;
            }, event);

        BindEvent<MouseMovedEvent>([&](MouseMovedEvent& e)
            {
                m_PrevMousePos = m_MousePos;
                m_MousePos.x = e.GetMouseX();
                m_MousePos.y = e.GetMouseY();
                m_MousePosUpdated = true;
            }, event);

        BindEvent<MouseEnteredEvent>([&](MouseEnteredEvent& e)
            {
                for (auto& b : m_KeyMap)
                    b.second = false;
                for (auto& b : m_MouseMap)
                    b.second = false;
            }, event);

        BindEvent<MouseExitedEvent>([&](MouseExitedEvent& e)
            {
                for (auto& b : m_KeyMap)
                    b.second = false;
                for (auto& b : m_MouseMap)
                    b.second = false;
            }, event);

        // Still push all these inputs to other layers
        return false;
    }
}