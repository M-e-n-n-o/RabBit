#pragma once
#include "RabBitCommon.h"
#include "ComponentRegister.h"

namespace RB::Entity
{
	class ObjectComponent;

	class GameObject
	{
	public:
		GameObject(ComponentRegister* reg);
		~GameObject();

		void Update();

		template<class T, typename... Args>
		T* AddComponent(Args... args);

		template<class T>
		T* GetComponent(uint32_t index = 0);

		void AppendComponentsWithTypeOf(ComponentID comp_id, List<const ObjectComponent*>& list) const;

	private:
		UnorderedMap<ComponentID, List<ObjectComponent*>> m_Components;

		ComponentRegister* m_Register;
	};

	template<class T, typename... Args>
	inline T* GameObject::AddComponent(Args... args)
	{
		T* comp = new T(args...);

		ComponentID tag = m_Register->RegisterComponent<T>();

		auto itr = m_Components.find(tag);
		if (itr == m_Components.end())
		{
			List<ObjectComponent*> list;
			list.push_back(comp);

			m_Components.emplace(tag, list);
		}
		else
		{
			itr->second.push_back(comp);
		}

		comp->OnAttachedToGameObject(this);
		return comp;
	}

	template<class T>
	T* GameObject::GetComponent(uint32_t index)
	{
		ComponentID id = m_Register->GetComponentID<T>();

		auto itr = m_Components.find(id);

		if (itr == m_Components.end())
		{
			return nullptr;
		}

		index = Math::Min(index, (uint32_t)(itr->second.size() - 1));

		return (T*) itr->second[index];
	}
}