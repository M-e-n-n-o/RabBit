#pragma once

#include "utils/Container.h"

namespace RB::Entity
{
    class Scene;
    class GameObject;
    class Transform;
    class Mesh;
    class Material;

    namespace SceneUtils
    {
        struct SpawnedModel
        {
            GameObject*         root            = nullptr;
            Transform*          rootTransform   = nullptr;

            List<GameObject*>   nodeObjects;                // One per LoadedModel::nodes entry (same index)
            List<Transform*>    nodeTransforms;             // Same, these are what an animation player would drive
            List<Mesh*>         meshes;                     // One per submodel that got spawned !!!MAKE SURE TO DELETE THESE!!!
        };

        SpawnedModel SpawnModel(Scene* scene, const char* model_path, const char* texture_path_prefix = "");
    }
}