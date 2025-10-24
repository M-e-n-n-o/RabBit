#pragma once
#include "RabBitCommon.h"
#include "BaseClasses.h"
#include "components/Transform.h"

namespace RB::Entity
{
	class ObjectComponent;

	template<typename Tuple, typename F, std::size_t... I>
	void ForEachTypeImpl(F&& f, std::index_sequence<I...>)
	{
		(void)std::initializer_list<int>{(f(std::type_index(typeid(std::tuple_element_t<I, Tuple>))), 0)...};
	}

	template<typename Tuple, typename F>
	void ForEachType(F&& f) 
	{
		ForEachTypeImpl<Tuple>(std::forward<F>(f), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
	}

	class GameObject
	{
	public:
		GameObject();
		~GameObject();

		void Update();

		template<class T, typename... Args>
		T* AddComponent(Args&&... args);

		// Check if a component of this type or any of its bases already exists
		template<typename T>
		bool HasComponent() const;

		template<class T>
		T* GetComponent() const;
		
		template<typename T>
		bool RemoveComponent();

		// Pass in nullptr to detach the parent
		void SetParent(GameObject* new_parent);
		GameObject* GetParent() const;

		const UnorderedSet<GameObject*>& GetChildren() const;

	private:

		void OnNewChildAttached(GameObject* obj);
		void OnChildDetached(GameObject* obj);

		GameObject*										m_Parent;
		UnorderedSet<GameObject*>						m_Children;
		List<ObjectComponent*>							m_Components;
		UnorderedMap<std::type_index, ObjectComponent*> m_Map;
	};

	template<class T, typename... Args>
	inline T* GameObject::AddComponent(Args&&... args)
	{
		RB_STATIC_ASSERT(std::is_base_of_v<ObjectComponent, T>, "T must derive from ObjectComponent");

        if (HasComponent<T>())
		{
			RB_LOG_ERROR("Failed to insert %s ObjectComponent! The GameObject already has this component type", typeid(T).name());
            return nullptr;
        }

		T* comp = new T(std::forward<Args>(args)...);
		m_Components.push_back(comp);

        // Register under the concrete type
		m_Map[std::type_index(typeid(T))] = comp;

        // Register under all declared base types
        using Bases = typename BaseClasses<T>::type;
		ForEachType<Bases>([this, comp](std::type_index base) {
			m_Map[base] = comp;
		});

		comp->OnAttachedToGameObject(this);

        return comp;
	}

	template<typename T>
	bool GameObject::HasComponent() const 
	{
		if (m_Map.find(std::type_index(typeid(T))) != m_Map.end())
			return true;

		bool found = false;
		using Bases = typename BaseClasses<T>::type;
		ForEachType<Bases>([&](std::type_index baseTI) {
			if (m_Map.find(baseTI) != m_Map.end())
				found = true;
		});

		return found;
	}

	template<class T>
	T* GameObject::GetComponent() const
	{
		RB_STATIC_ASSERT(std::is_base_of_v<ObjectComponent, T>, "T must derive from ObjectComponent");

		auto it = m_Map.find(std::type_index(typeid(T)));
		if (it == m_Map.end())
		{
			if constexpr (std::is_same<T, Transform>::value)
			{
				// A gameobject should always have a transform
				return (T*)GetDefaultTransform();
			}

			return nullptr;
		}

		return static_cast<T*>(it->second);
	}

	template<typename T>
	bool GameObject::RemoveComponent() 
	{
		ObjectComponent* comp = GetComponent<T>();

		if (!comp)
			return false;

		auto itr = std::find(m_Components.begin(), m_Components.end(), comp);
		m_Components.erase(itr);

		// Remove all mappings to this component
		for (auto it = m_Map.begin(); it != m_Map.end();) 
		{
			if (it->second == comp) 
				it = m_Map.erase(it);
			else 
				++it;
		}

		delete comp;
		return true;
	}
}