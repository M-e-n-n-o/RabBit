#include "RabBitCommon.h"
#include "Input.h"

#include "events/KeyEvent.h"
#include "events/MouseEvent.h"

namespace RB::Events
{
    InputLayer* InputLayer::s_Instance = nullptr;

    InputLayer::InputLayer()
        : ApplicationLayer("InputLayer")
        , m_ScrollAccum(0.0f)
        , m_ScrollDelta(0.0f)
        , m_HasMousePos(false)
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
        return l->m_MouseDelta;
    }

    float InputLayer::GetMouseScrollDelta()
    {
        InputLayer* l = GetInstance();
        return l->m_ScrollDelta;
    }

    void InputLayer::OnUpdate(float delta_time)
    {
        m_MouseDelta = m_MouseDeltaAccum;
        m_MouseDeltaAccum = { 0.0f, 0.0f };

        m_ScrollDelta = m_ScrollAccum;
        m_ScrollAccum = 0.0f;
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
                m_ScrollAccum += e.GetDeltaY();
            }, event);

        BindEvent<MouseMovedEvent>([&](MouseMovedEvent& e)
            {
                if (e.IsRawMovement())
                {
                    // For raw events, X/Y hold the relative movement from the device, not a position, so accumulate them as-is
                    m_MouseDeltaAccum = m_MouseDeltaAccum + Math::Float2(e.GetMouseX(), e.GetMouseY());
                }
                else
                {
                    // Absolute cursor position only
                    m_MousePos = Math::Float2(e.GetMouseX(), e.GetMouseY());
                    m_HasMousePos = true;
                }
            }, event);

        BindEvent<MouseEnteredEvent>([&](MouseEnteredEvent& e)
            {
                m_HasMousePos = false;
                for (auto& b : m_KeyMap)
                    b.second = false;
                for (auto& b : m_MouseMap)
                    b.second = false;
            }, event);

        BindEvent<MouseExitedEvent>([&](MouseExitedEvent& e)
            {
                m_HasMousePos = false;
                for (auto& b : m_KeyMap)
                    b.second = false;
                for (auto& b : m_MouseMap)
                    b.second = false;
            }, event);

        // Still push all these inputs to other layers
        return false;
    }
}