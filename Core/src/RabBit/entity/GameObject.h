#pragma once
#include "RabBitCommon.h"
#include "ComponentRegister.h"

namespace RB::Entity
{
	class ObjectComponent;

	class GameObject
	{
	public:
		GameObject();
		~GameObject();

		void Update();

		template<class T, typename... Args>
		T* AddComponent(Args... args);

		void AppendComponentsWithTypeOf(ComponentID comp_id, List<ObjectComponent*>& list) const;

	private:
		UnorderedMap<ComponentID, List<ObjectComponent*>> m_Components;
	};

	template<class T, typename... Args>
	inline T* GameObject::AddComponent(Args... args)
	{
		ObjectComponent* comp = new T(args...);

		ComponentID tag = g_ComponentRegister->RegisterComponent<T>();

		auto itr = m_Components.find(tag);
		if (itr == m_Components.end())
		{
			m_Components.emplace({ tag, List<T>(comp) });
		}
		else
		{
			itr->second.push_back(comp);
		}

		comp->OnAttachedToGameObject(this);
		return comp;
	}
}