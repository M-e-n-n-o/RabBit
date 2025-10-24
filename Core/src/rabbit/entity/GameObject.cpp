#include "RabBitCommon.h"
#include "GameObject.h"
#include "ObjectComponent.h"

namespace RB::Entity
{
    GameObject::GameObject()
        : m_Parent(nullptr)
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

    void GameObject::Update()
    {
        for (ObjectComponent* comp : m_Components)
        {
            comp->Update();
        }
    }

    void GameObject::SetParent(GameObject* new_parent)
    {
        if (new_parent == nullptr)
        {
            if (m_Parent != nullptr)
            {
                m_Parent->OnChildDetached(this);
            }
        }
        else
        {
            new_parent->OnNewChildAttached(this);
        }

        m_Parent = new_parent;
    }

    GameObject* GameObject::GetParent() const
    {
        return m_Parent;
    }

    const UnorderedSet<GameObject*>& GameObject::GetChildren() const
    {
        return m_Children;
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