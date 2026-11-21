#pragma once

#include "utils/Container.h"

namespace RB { struct LoadedMesh; struct LoadedModel; }

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

            List<GameObject*>   nodeObjects;                // One per LoadedMesh::nodes entry (same index)

            // Memory management !!!MAKE SURE TO DELETE THESE!!!
            List<Mesh*>         meshes;                     // One per spawned LoadedMesh::submeshes entry
            List<Animation*>    animations;                 // One per LoadedMesh::animations entry
        };

        // Loads a converted model together with its mesh and materials and spawns it
        SpawnedModel SpawnModel(Scene* scene, const char* model_path, bool converted_textures = true, const char* texture_path_prefix = "");

        // submesh_materials has one entry per mesh submesh, submeshes with a nullptr material are skipped
        SpawnedModel SpawnModel(Scene* scene, const LoadedMesh* loaded_mesh, const List<Material*>& submesh_materials);

        // Loads every material the model references, returns one material per submesh (same order as LoadedModel::submeshMaterials).
        // Submeshes without (a loadable) material get a default material. !!!The caller owns the returned materials!!!
        List<Material*> LoadModelMaterials(const LoadedModel& loaded_model, bool converted_textures = true, const char* texture_path_prefix = "");
    }
}