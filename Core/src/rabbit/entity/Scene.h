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

        GameObject* CreateGameObject(const char* name = "GameObject");
        void RemoveGameObject(GameObject* obj);

        void UpdateScene();

        List<GameObject*>& GetGameObjects();

        template<class T>
        const List<const ObjectComponent*> GetComponentsWithTypeOf() const;

    private:
        List<GameObject*>  m_GameObjects;
    };

    template<class T>
    inline const List<const ObjectComponent*> Scene::GetComponentsWithTypeOf() const
    {
        List<const ObjectComponent*> list;
        for (const GameObject* obj : m_GameObjects)
        {
            ObjectComponent* comp = obj->GetComponent<T>();
            if (comp)
                list.push_back(comp);
        }

        return list;
    }
}