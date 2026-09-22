#include "SceneUtils.h"
#include "Scene.h"
#include "GameObject.h"
#include "app/AssetLoader.h"
#include "components/Transform.h"
#include "components/Mesh.h"

namespace RB::Entity
{
    using namespace SceneUtils;

    SpawnedModel SceneUtils::SpawnModel(Scene* scene, const char* model_path, const char* texture_path_prefix)
    {
        LoadedModel model;
        bool success = AssetLoader::LoadConvertedModel(model_path, &model);

        List<Material*> materials;
        for (int i = 0; i < model.diffuseColorTextures.size(); i++)
        {
            materials.push_back(new Material((texture_path_prefix + model.diffuseColorTextures[i]).c_str(), false));
        }

        SpawnedModel result;
        result.nodeObjects.resize(model.nodes.size());
        result.nodeTransforms.resize(model.nodes.size());

        // Nodes (The loader guarantees that parents come before their children)
        for (size_t i = 0; i < model.nodes.size(); i++)
        {
            const LoadedModel::Node& node = model.nodes[i];

            GameObject* object = scene->CreateGameObject();
            Transform* transform = object->AddComponent<Transform>();

            if (node.parent >= 0)
                object->SetParent(result.nodeObjects[node.parent]);

            // Local transform
            transform->position = node.translation;
            transform->rotation = node.rotation;
            transform->scale    = node.scale;

            result.nodeObjects[i]    = object;
            result.nodeTransforms[i] = transform;
        }

        if (!result.nodeObjects.empty())
        {
            result.root          = result.nodeObjects[0];
            result.rootTransform = result.nodeTransforms[0];
        }

        // Submodels
        std::vector<bool> has_renderer(model.nodes.size(), false);
        for (const LoadedModel::Submodel& submodel : model.models)
        {
            if (submodel.positions.empty() || submodel.nodeIndex >= result.nodeObjects.size())
                continue;

            Material* material = materials[submodel.diffuseTexIndex];
            if (material == nullptr)
                continue;

            Mesh* mesh = new Mesh("Mesh", submodel);
            result.meshes.push_back(mesh);

            GameObject* object = result.nodeObjects[submodel.nodeIndex];

            if (has_renderer[submodel.nodeIndex])
            {
                // One renderer per object: extra submodels on the same node (e.g. one per material) go into an identity child
                GameObject* child = scene->CreateGameObject();
                Transform* child_transform = child->AddComponent<Transform>();
                child->SetParent(object);

                child_transform->position = Math::Float3(0.0f, 0.0f, 0.0f);
                child_transform->rotation = Math::Quaternion::FromEuler(0.0f, 0.0f, 0.0f);
                child_transform->scale    = Math::Float3(1.0f);

                object = child;
            }

            has_renderer[submodel.nodeIndex] = true;
            object->AddComponent<MeshRenderer>(mesh, material);
        }

        return result;
    }
}

