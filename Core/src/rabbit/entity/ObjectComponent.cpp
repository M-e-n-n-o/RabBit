#include "RabBitCommon.h"
#include "ObjectComponent.h"

namespace RB::Entity
{
    ObjectComponent::ObjectComponent()
        : m_GameObject(nullptr)
        , m_Enabled(false)
    {}

    void ObjectComponent::OnAttachedToGameObject(GameObject* obj)
    {
        m_GameObject = obj;
        m_Enabled = true;
        OnAttached();
    }
}