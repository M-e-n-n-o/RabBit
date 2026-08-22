#pragma once
#include "RabBitCommon.h"
#include "BaseClasses.h"

namespace RB::Entity
{
    class GameObject;

    // Macro to declare base classes for derived components.
    #define REGISTER_COMP_BASES(Derived, ...) template<> struct BaseClasses<Derived> { using type = std::tuple<__VA_ARGS__>; }

    class ObjectComponent
    {
    public:
        ObjectComponent();
        virtual ~ObjectComponent() = default;

        virtual void OnAttached() {}

        virtual void Update() {}
        virtual void OnChildAttached(GameObject* obj) {}
        virtual void OnChildDettached(GameObject* obj) {}

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