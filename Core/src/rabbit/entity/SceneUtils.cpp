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

    List<Material*> SceneUtils::LoadModelMaterials(const LoadedModel& loaded_model, bool converted_textures, const char* texture_path_prefix)
    {
        const std::string prefix = texture_path_prefix ? texture_path_prefix : "";

        // One engine material per material file, shared by every submesh that uses it
        List<Material*> materials(loaded_model.materialPaths.size(), nullptr);
        for (size_t i = 0; i < loaded_model.materialPaths.size(); i++)
        {
            LoadedMaterial loaded_material;
            if (!AssetLoader::LoadConvertedMaterial(loaded_model.materialPaths[i].c_str(), &loaded_material))
                continue;

            Material* material = new Material();
            if (!loaded_material.diffuseTexture.empty())
                material->LoadAlbedoTexture((prefix + loaded_material.diffuseTexture).c_str(), converted_textures);
            if (!loaded_material.normalTexture.empty())
                material->LoadNormalTexture((prefix + loaded_material.normalTexture).c_str(), converted_textures);

            materials[i] = material;
        }

        Material* default_material = nullptr;

        List<Material*> submesh_materials(loaded_model.submeshMaterials.size(), nullptr);
        for (size_t i = 0; i < loaded_model.submeshMaterials.size(); i++)
        {
            const uint32_t index = loaded_model.submeshMaterials[i];
            Material* material = (index != LoadedModel::kNoMaterial) ? materials[index] : nullptr;

            if (material == nullptr)
            {
                if (default_material == nullptr)
                    default_material = new Material();

                material = default_material;
            }

            submesh_materials[i] = material;
        }

        return submesh_materials;
    }

    SpawnedModel SceneUtils::SpawnModel(Scene* scene, const char* model_path, bool converted_textures, const char* texture_path_prefix)
    {
        LoadedModel model;
        if (!AssetLoader::LoadConvertedModel(model_path, &model))
            return SpawnedModel();

        LoadedMesh mesh;
        if (!AssetLoader::LoadConvertedMesh(model.meshPath.c_str(), &mesh))
            return SpawnedModel();

        if (mesh.submeshes.size() != model.submeshMaterials.size())
        {
            RB_LOG_ERROR(LOGTAG_MAIN, "Model \"%s\" expects %d submeshes but its mesh has %d, convert it again", model_path, (int)model.submeshMaterials.size(), (int)mesh.submeshes.size());
            return SpawnedModel();
        }

        const List<Material*> submesh_materials = LoadModelMaterials(model, converted_textures, texture_path_prefix);

        return SpawnModel(scene, &mesh, submesh_materials);
    }

    SpawnedModel SceneUtils::SpawnModel(Scene* scene, const LoadedMesh* model, const List<Material*>& submesh_materials)
    {
        SpawnedModel result;
        result.nodeObjects.resize(model->nodes.size());

        List<Transform*> node_transforms(model->nodes.size());

        // Nodes (The loader guarantees that parents come before their children)
        for (size_t i = 0; i < model->nodes.size(); i++)
        {
            const LoadedMesh::Node& node = model->nodes[i];

            GameObject* object = scene->CreateGameObject();
            Transform* transform = object->AddComponent<Transform>();

            if (node.parent >= 0)
                object->SetParent(result.nodeObjects[node.parent]);

            // Local transform
            transform->SetPosition(node.translation);
            transform->SetRotation(node.rotation);
            transform->SetScale(node.scale);

            result.nodeObjects[i] = object;
            node_transforms[i] = transform;
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

        // Submeshes
        std::vector<bool> has_renderer(model->nodes.size(), false);
        for (size_t submesh_index = 0; submesh_index < model->submeshes.size(); submesh_index++)
        {
            const LoadedMesh::Submesh& submodel = model->submeshes[submesh_index];
            if (submodel.positions.empty() || submodel.nodeIndex >= result.nodeObjects.size())
                continue;

            Material* material = submesh_index < submesh_materials.size() ? submesh_materials[submesh_index] : nullptr;
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

                object = child;
            }

            has_renderer[submodel.nodeIndex] = true;

            if (submodel.isSkinned)
                object->AddComponent<SkinnedMeshRenderable>(mesh, material, node_transforms, submodel.skinBones);
            else
                object->AddComponent<MeshRenderable>(mesh, material);
        }

        // Animations
        if (result.animator)
        {
            for (const LoadedMesh::Animation& loaded_anim : model->animations)
            {
                Animation* anim = new Animation(loaded_anim.name.c_str(), loaded_anim.duration);
                result.animations.push_back(anim);

                for (const LoadedMesh::AnimationChannel& loaded_channel : loaded_anim.channels)
                {
                    for (const LoadedMesh::Float3Key& loaded_trans_key : loaded_channel.translations)
                        anim->AddKey(loaded_channel.nodeIndex, kAnimProperty_Position, loaded_trans_key.time, loaded_trans_key.value);

                    for (const LoadedMesh::QuatKey& loaded_quat_key : loaded_channel.rotations)
                        anim->AddKey(loaded_channel.nodeIndex, kAnimProperty_Rotation, loaded_quat_key.time, loaded_quat_key.value);

                    for (const LoadedMesh::Float3Key& loaded_scale_key : loaded_channel.scales)
                        anim->AddKey(loaded_channel.nodeIndex, kAnimProperty_Scale, loaded_scale_key.time, loaded_scale_key.value);
                }

                result.animator->AddAnimation(anim);
            }
        }

        return result;
    }
}

