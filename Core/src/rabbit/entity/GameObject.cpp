#include "RabBitCommon.h"
#include "GameObject.h"
#include "ObjectComponent.h"

namespace RB::Entity
{
    GameObject::GameObject(const char* name)
        : m_Name(name)
        , m_Parent(nullptr)
    {
    }

    GameObject::~GameObject()
    {
        SetParent(nullptr);

        for (ObjectComponent* comp : m_Components)
        {
            delete comp;
        }
    }

    void GameObject::OnUpdate(float delta_time)
    {
        for (ObjectComponent* comp : m_Components)
        {
            comp->OnUpdate(delta_time);
        }
    }

    void GameObject::SetParent(GameObject* new_parent)
    {
        if (m_Parent == new_parent)
            return;

        if (m_Parent != nullptr)
            m_Parent->OnChildDetached(this);

        m_Parent = new_parent;

        OnNewParent();

        if (new_parent != nullptr)
            new_parent->OnNewChildAttached(this);
    }

    GameObject* GameObject::GetParent() const
    {
        return m_Parent;
    }

    const UnorderedSet<GameObject*>& GameObject::GetChildren() const
    {
        return m_Children;
    }

    void GameObject::OnNewParent()
    {
        for (ObjectComponent* comp : m_Components)
        {
            comp->OnNewParent(m_Parent);
        }
    }

    void GameObject::OnNewChildAttached(GameObject* obj)
    {
        m_Children.insert(obj);

        for (ObjectComponent* comp : m_Components)
        {
            comp->OnChildAttached(obj);
        }
    }

    void GameObject::OnChildDetached(GameObject* obj)
    {
        m_Children.erase(obj);

        for (ObjectComponent* comp : m_Components)
        {
            comp->OnChildDettached(obj);
        }
    }
}