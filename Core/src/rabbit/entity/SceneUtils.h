#pragma once

#include "utils/Container.h"

namespace RB { class LoadedModel; }

namespace RB::Entity
{
    class Scene;
    class GameObject;
    class Transform;
    class Mesh;
    class Material;
    class Animator;
    class Animation;

    namespace SceneUtils
    {
        struct SpawnedModel
        {
            GameObject*         root            = nullptr;
            Transform*          rootTransform   = nullptr;
            Animator*           animator        = nullptr;

            List<GameObject*>   nodeObjects;                // One per LoadedModel::nodes entry (same index)

            // Memory management !!!MAKE SURE TO DELETE THESE!!!
            List<Mesh*>         meshes;                     // One per LoadedModel::models entry
            List<Animation*>    animations;                 // One per LoadedModel::animations entry
        };

        SpawnedModel SpawnModel(Scene* scene, const char* model_path, const char* texture_path_prefix = "");
        SpawnedModel SpawnModel(Scene* scene, const LoadedModel* loaded_model, const List<Material*>& materials);
    }
}