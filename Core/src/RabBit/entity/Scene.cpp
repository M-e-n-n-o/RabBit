#include "RabBitCommon.h"
#include "Scene.h"
#include "GameObject.h"
#include "ComponentRegister.h"

namespace RB::Entity
{
	Scene::Scene()
	{
		m_ComponentRegister = new ComponentRegister();
	}

	Scene::~Scene()
	{
		for (int i = 0; i < m_GameObjects.size(); ++i)
		{
			delete m_GameObjects[i];
		}

		delete m_ComponentRegister;
	}

	GameObject* Scene::CreateGameObject()
	{
		GameObject* obj = new GameObject(m_ComponentRegister);
		m_GameObjects.push_back(obj);

		return obj;
	}
	
	void Scene::RemoveGameObject(GameObject* obj)
	{
		auto itr = std::find(m_GameObjects.begin(), m_GameObjects.end(), obj);
		
		if (itr == m_GameObjects.end())
		{
			RB_LOG_WARN(LOGTAG_ENTITY, "Could not find the game object to delete");
			return;
		}

		m_GameObjects.erase(itr);
		delete obj;
	}

	void Scene::UpdateScene()
	{
		for (GameObject* obj : m_GameObjects)
		{
			obj->Update();
		}
	}

	List<GameObject*> Scene::GetGameObjects() const
	{
		return m_GameObjects;
	}
}