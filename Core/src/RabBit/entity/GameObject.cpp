#include "RabBitCommon.h"
#include "GameObject.h"
#include "ObjectComponent.h"

namespace RB::Entity
{
	GameObject::GameObject()
	{
	}

	GameObject::~GameObject()
	{
		for (auto itr = m_Components.begin(); itr != m_Components.end(); ++itr)
		{
			for (int i = 0; i < itr->second.size(); ++i)
			{
				delete itr->second[i];
				i--;
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
	void GameObject::AppendComponentsWithTypeOf(ComponentID comp_id, List<ObjectComponent*>& list) const
	{
		auto itr = m_Components.find(comp_id);
		if (itr == m_Components.end())
		{
			return;
		}

		list.insert(list.end(), itr->second.begin(), itr->second.end());
	}
}