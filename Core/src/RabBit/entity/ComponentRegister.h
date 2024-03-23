#pragma once
#include "RabBitCommon.h"

namespace RB::Entity
{
	class ObjectComponent;

	using ComponentID = int32_t;

	class ComponentRegister
	{
	public:
		ComponentRegister();

		template<class T>
		ComponentID RegisterComponent();

		template<class T>
		ComponentID GetComponentID();

	private:
		UnorderedMap<const char*, ComponentID> m_IDs;
		ComponentID m_NextID;
	};

	template<class T>
	inline ComponentID ComponentRegister::RegisterComponent()
	{
		ComponentID id = GetComponentID<T>();
		if (id != -1)
		{
			return id;
		}

		id = m_NextID++;
		m_IDs.emplace({ T::GetComponentTag(), id });
		return id;
	}

	template<class T>
	inline ComponentID ComponentRegister::GetComponentID()
	{
		auto itr = m_IDs.find(T::GetComponentTag());
		return itr != m_IDs.end() ? itr->second : -1;
	}

	extern ComponentRegister* g_ComponentRegister;
}