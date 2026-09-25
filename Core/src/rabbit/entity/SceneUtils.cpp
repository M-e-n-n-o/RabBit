#include "SceneUtils.h"
#include "Scene.h"
#include "GameObject.h"
#include "app/AssetLoader.h"
#include "components/Transform.h"
#include "components/Mesh.h"
#include "components/Animation.h"

namespace RB::Entity
{
    using namespace SceneUtils;

    SpawnedModel SceneUtils::SpawnModel(Scene* scene, const char* model_path, const char* texture_path_prefix)
    {
        LoadedModel model;
        bool success = AssetLoader::LoadConvertedModel(model_path, &model);
        if (!success)
        {
            return SpawnedModel();
        }

        List<Material*> materials;
        for (int i = 0; i < model.diffuseColorTextures.size(); i++)
        {
            materials.push_back(new Material((texture_path_prefix + model.diffuseColorTextures[i]).c_str(), false));
        }

        return SpawnModel(scene, &model, materials);
    }

    SpawnedModel SceneUtils::SpawnModel(Scene* scene, const LoadedModel* model, const List<Material*>& materials)
    {
        SpawnedModel result;
        result.nodeObjects.resize(model->nodes.size());

        // Nodes (The loader guarantees that parents come before their children)
        for (size_t i = 0; i < model->nodes.size(); i++)
        {
            const LoadedModel::Node& node = model->nodes[i];

            GameObject* object = scene->CreateGameObject();
            Transform* transform = object->AddComponent<Transform>();

            if (node.parent >= 0)
                object->SetParent(result.nodeObjects[node.parent]);

            // Local transform
            transform->position = node.translation;
            transform->rotation = node.rotation;
            transform->scale    = node.scale;

            result.nodeObjects[i]    = object;
        }

        if (!result.nodeObjects.empty())
        {
            result.root = result.nodeObjects[0];
            result.rootTransform = result.root->GetComponent<Transform>();

            if (!model->animations.empty())
            {
                result.animator = result.root->AddComponent<Animator>(result.nodeObjects);
            }
        }

        // Submodels
        std::vector<bool> has_renderer(model->nodes.size(), false);
        for (const LoadedModel::Submodel& submodel : model->models)
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
            object->AddComponent<MeshRenderable>(mesh, material);
        }

        // Animations
        if (result.animator)
        {
            for (const LoadedModel::Animation& loaded_anim : model->animations)
            {
                Animation* anim = new Animation(loaded_anim.name.c_str(), loaded_anim.duration);
                result.animations.push_back(anim);

                for (const LoadedModel::AnimationChannel& loaded_channel : loaded_anim.channels)
                {
                    for (const LoadedModel::Float3Key& loaded_trans_key : loaded_channel.translations)
                        anim->AddKey(loaded_channel.nodeIndex, kAnimProperty_Position, loaded_trans_key.time, loaded_trans_key.value);

                    for (const LoadedModel::QuatKey& loaded_quat_key : loaded_channel.rotations)
                        anim->AddKey(loaded_channel.nodeIndex, kAnimProperty_Rotation, loaded_quat_key.time, loaded_quat_key.value);

                    for (const LoadedModel::Float3Key& loaded_scale_key : loaded_channel.scales)
                        anim->AddKey(loaded_channel.nodeIndex, kAnimProperty_Scale, loaded_scale_key.time, loaded_scale_key.value);
                }

                result.animator->AddAnimation(anim);
            }
        }

        return result;
    }
}

