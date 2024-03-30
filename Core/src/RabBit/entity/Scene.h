#pragma once
#include "RabBitCommon.h"
#include "GameObject.h"

namespace RB::Entity
{
	class Scene
	{
	public:
		Scene();
		~Scene();

		GameObject* CreateGameObject();
		void RemoveGameObject(GameObject* obj);

		void UpdateScene();

		List<GameObject*> GetGameObjects() const;

		template<class T>
		List<const ObjectComponent*> GetComponentsWithTypeOf() const;

	private:
		List<GameObject*> m_GameObjects;
	};

	template<class T>
	inline List<const ObjectComponent*> Scene::GetComponentsWithTypeOf() const
	{
		List<const ObjectComponent*> list;

		ComponentID id = g_ComponentRegister->GetComponentID<T>();

		if (id == -1)
		{
			RB_LOG_WARN(LOGTAG_ENTITY, "Component of type %s is not registered yet", T::GetComponentTag());
			return list;
		}

		for (const GameObject* obj : m_GameObjects)
		{
			obj->AppendComponentsWithTypeOf(id, list);
		}

		return list;
	}
}