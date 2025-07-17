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

    void GameObject::SetParent(GameObject* obj)
    {
        if (obj == nullptr)
        {
            if (m_Parent != nullptr)
            {
                m_Parent->OnChildDetached(this);
            }
        }
        else
        {
            m_Parent->OnNewChildAttached(this);
        }

        m_Parent = obj;
    }

    GameObject* GameObject::GetParent()
    {
        return m_Parent;
    }

    void GameObject::OnNewChildAttached(GameObject* obj)
    {
        m_Children.insert(obj);
    }

    void GameObject::OnChildDetached(GameObject* obj)
    {
        m_Children.erase(obj);
    }
}