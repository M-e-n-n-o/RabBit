#pragma once
#include "RabBitCommon.h"
#include "ComponentRegister.h"
#include "components/Transform.h"

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
		bool HasComponent(uint32_t index = 0) const;

		template<class T>
		T* GetComponent(uint32_t index = 0) const;

		void AppendComponentsWithTypeOf(ComponentID comp_id, List<const ObjectComponent*>& list) const;
		
		// Pass in nullptr to detach the parent
		void SetParent(GameObject* new_parent);
		GameObject* GetParent() const;

		const UnorderedSet<GameObject*>& GetChildren() const;

	private:
		void OnNewChildAttached(GameObject* obj);
		void OnChildDetached(GameObject* obj);

		GameObject* m_Parent;
		UnorderedSet<GameObject*> m_Children;

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
	bool GameObject::HasComponent(uint32_t index) const
	{
		ComponentID id = m_Register->GetComponentID<T>();

		auto itr = m_Components.find(id);

		if (itr == m_Components.end())
		{
			return false;
		}

		return index < itr->second.size();
	}

	template<class T>
	T* GameObject::GetComponent(uint32_t index) const
	{
		ComponentID id = m_Register->GetComponentID<T>();

		auto itr = m_Components.find(id);

		if (itr == m_Components.end())
		{
			if constexpr (std::is_same<T, Transform>::value)
			{
				// A gameobject should always have a transform
				return (T*)GetDefaultTransform();
			}

			return nullptr;
		}

		if (index >= itr->second.size())
		{
			return nullptr;
		}

		return (T*) itr->second[index];
	}
}