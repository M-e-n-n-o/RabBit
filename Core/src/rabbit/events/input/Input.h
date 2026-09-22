#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"
#include "math/Vector.h"

#include "app/ApplicationLayer.h"

namespace RB::Events
{
    class InputLayer : public ApplicationLayer
    {
    public:
        InputLayer();

        static bool IsKeyDown(const KeyCode& key);

        static bool IsMouseKeyDown(const MouseCode& mouse_button);

        static Math::Float2 GetMousePos();
        static Math::Float2 GetMousePosDelta();

        static float GetMouseScrollDelta();

    private:
        void OnUpdate(float delta_time) override;
        bool OnEvent(Event& event) override;

        static InputLayer* GetInstance() { return s_Instance; }

        UnorderedMap<KeyCode, bool>   m_KeyMap;
        UnorderedMap<MouseCode, bool> m_MouseMap;

        Math::Float2                  m_MousePos;
        Math::Float2                  m_PrevMousePos;
        bool                          m_MousePosUpdated;

        float                         m_MouseScrollDelta;
        bool                          m_MouseScrollUpdated;

        static InputLayer*            s_Instance;
    };

    using Input = InputLayer;
}