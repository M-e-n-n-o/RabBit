#pragma once
#include "RabBitCommon.h"

namespace RB::Entity
{
    #define DEFINE_COMP_TAG(name) static char* GetComponentTag() { return (name); }

    class GameObject;

    class ObjectComponent
    {
    public:
        virtual ~ObjectComponent() = default;

        virtual void Update() {}

        GameObject* GetGameObject() const { return m_GameObject; }

        bool IsEnabled() const { return m_Enabled; }
        void SetEnabled(bool enabled) { m_Enabled = enabled; }

    protected:
        GameObject* m_GameObject;
        bool        m_Enabled;

    private:
        friend class GameObject;

        void OnAttachedToGameObject(GameObject* obj);
    };
}