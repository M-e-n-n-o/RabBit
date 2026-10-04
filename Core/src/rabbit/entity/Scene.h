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

        void UpdateScene(float delta_time);

        const List<GameObject*>& GetGameObjects() const;

        template<class T>
        const List<const T*> GetComponentsWithTypeOf() const;

    private:
        List<GameObject*>  m_GameObjects;
    };

    template<class T>
    inline const List<const T*> Scene::GetComponentsWithTypeOf() const
    {
        List<const T*> list;
        for (const GameObject* obj : m_GameObjects)
        {
            T* comp = obj->GetComponent<T>();
            if (comp)
                list.push_back(comp);
        }

        return list;
    }
}