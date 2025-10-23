#include "RabBitCommon.h"
#include "GameObject.h"
#include "ObjectComponent.h"

namespace RB::Entity
{
    GameObject::GameObject(ComponentRegister* reg)
        : m_Register(reg)
        , m_Parent(nullptr)
    {
    }

    GameObject::~GameObject()
    {
        SetParent(nullptr);

        for (auto itr = m_Components.begin(); itr != m_Components.end(); ++itr)
        {
            for (int i = 0; i < itr->second.size(); ++i)
            {
                delete itr->second[i];
            }
        }
    }

    void GameObject::Update()
    {
        for (auto itr = m_Components.begin(); itr != m_Components.end(); ++itr)
        {
            for (ObjectComponent* comp : itr->second)
            {
                comp->Update();
            }
        }
    }

    void GameObject::AppendComponentsWithTypeOf(ComponentID comp_id, List<const ObjectComponent*>& list) const
    {
        auto itr = m_Components.find(comp_id);
        if (itr == m_Components.end())
        {
            return;
        }

        list.insert(list.end(), itr->second.begin(), itr->second.end());
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

        for (auto itr = m_Components.begin(); itr != m_Components.end(); ++itr)
        {
            for (ObjectComponent* comp : itr->second)
            {
                comp->OnChildAttached(obj);
            }
        }
    }

    void GameObject::OnChildDetached(GameObject* obj)
    {
        m_Children.erase(obj);

        for (auto itr = m_Components.begin(); itr != m_Components.end(); ++itr)
        {
            for (ObjectComponent* comp : itr->second)
            {
                comp->OnChildDettached(obj);
            }
        }
    }
}