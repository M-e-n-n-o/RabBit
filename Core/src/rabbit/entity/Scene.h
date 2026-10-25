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
        struct ComponentQueryCache
        {
            uint64_t                     structureVersion = UINT64_MAX;
            size_t                       objectCount      = 0;
            List<const ObjectComponent*> components;
        };

        List<GameObject*> m_GameObjects;

        // Scanning every GameObject per query is expensive, so keep the result per component type 
        // and only rescan when a component was added/removed or an object was destroyed.
        mutable UnorderedMap<std::type_index, ComponentQueryCache> m_QueryCache;
    };

    template<class T>
    inline const List<const T*> Scene::GetComponentsWithTypeOf() const
    {
        ComponentQueryCache& cache = m_QueryCache[std::type_index(typeid(T))];

        const uint64_t version = GameObject::GetStructureVersion();
        if (cache.structureVersion != version || m_GameObjects.size() != cache.objectCount)
        {
            cache.components.clear();
            for (const GameObject* obj : m_GameObjects)
            {
                T* comp = obj->GetComponent<T>();
                if (comp)
                    cache.components.push_back(comp);
            }

            cache.structureVersion = version;
            cache.objectCount      = m_GameObjects.size();
        }

        List<const T*> list;
        list.reserve(cache.components.size());
        for (const ObjectComponent* comp : cache.components)
            list.push_back(static_cast<const T*>(comp));

        return list;
    }
}