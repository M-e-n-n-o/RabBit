#include "RabBitCommon.h"
#include "Mesh.h"
#include "Transform.h"
#include "app/Application.h"
#include "app/AssetLoader.h"
#include "entity/Scene.h"
#include "entity/GameObject.h"
#include "entity/components/Camera.h"
#include "graphics/ResourceDefaults.h"

namespace RB::Entity
{
    Mesh::Mesh(const char* name, const LoadedModel::Submodel& submodel, bool allow_skinning)
    {
        char position_name[100];
        sprintf(position_name, "%s primary", name);

        char secondary_name[100];
        sprintf(secondary_name, "%s secondary", name);

        uint32_t position_size = sizeof(Math::Float3);
        uint32_t vertex_size = sizeof(LoadedModel::Vertex);

        m_VertexPack.primaryBuffer   = Graphics::VertexBuffer::Create(position_name,  RB::Graphics::TopologyType::TriangleList, submodel.positions.data(), position_size, position_size * submodel.positions.size());
        m_VertexPack.secondaryBuffer = Graphics::VertexBuffer::Create(secondary_name, RB::Graphics::TopologyType::TriangleList, submodel.vertices.data(), vertex_size, vertex_size * submodel.vertices.size());

        if (!submodel.indices.empty())
        {
            char index_name[100];
            sprintf(index_name, "%s indices", name);

            m_VertexPack.indexBuffer = Graphics::IndexBuffer::Create(index_name, submodel.indices.data(), submodel.indices.size());
        }

        if (allow_skinning && submodel.isSkinned)
        {
            char bones_name[100];
            sprintf(bones_name, "%s bones", name);

            uint32_t bones_size = sizeof(LoadedModel::SkinVertex);
            m_VertexPack.boneWeightBuffer = Graphics::VertexBuffer::Create(bones_name, RB::Graphics::TopologyType::TriangleList, submodel.skinVertices.data(), bones_size, bones_size * submodel.skinVertices.size());
        }

        m_ValidBounds = true;
        m_Bounds.min = submodel.minBounds;
        m_Bounds.max = submodel.maxBounds;
    }

    Mesh::Mesh(const char* name, const float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, const uint32_t* index_data, uint64_t index_data_count)
    {
        m_VertexPack.primaryBuffer = Graphics::VertexBuffer::Create(name, RB::Graphics::TopologyType::TriangleList, vertex_data, elements_per_vertex * sizeof(float), vertex_data_count * sizeof(float));

        if (index_data_count > 0)
        {
            std::string index_name = name;
            index_name += " index";

            m_VertexPack.indexBuffer = Graphics::IndexBuffer::Create(index_name.c_str(), index_data, index_data_count);
        }

        m_ValidBounds = false;
    }

    Material::Material()
    {
        m_AlbedoTex = Graphics::g_TexDefaultError;
        m_NormalTex = nullptr;
    }

    void Material::LoadAlbedoTexture(const char* name, LoadedImage* image)
    {
        m_AlbedoTex = nullptr;
        m_AlbedoTex = Graphics::Texture2D::Create(name, image->data, image->dataSize, image->format, image->width, image->height, false, false);
    }

    void Material::LoadAlbedoTexture(const char* file_name, bool converted_texture, TextureColorSpace color_space)
    {
        m_AlbedoTex = LoadTexture(file_name, converted_texture, color_space);

        if (m_AlbedoTex == nullptr)
            m_AlbedoTex = Graphics::g_TexDefaultError;
    }

    void Material::LoadNormalTexture(const char* name, LoadedImage* image)
    {
        m_NormalTex = Graphics::Texture2D::Create(name, image->data, image->dataSize, image->format, image->width, image->height, false, false);
    }

    void Material::LoadNormalTexture(const char* file_name, bool converted_texture, TextureColorSpace color_space)
    {
        m_NormalTex = LoadTexture(file_name, converted_texture, color_space);
    }

    Shared<Graphics::Texture2D> Material::LoadTexture(const char* file_name, bool converted_texture, TextureColorSpace color_space) const
    {
        LoadedImage img;

        bool success;
        if (converted_texture)
            success = AssetLoader::LoadConvertedTexture(file_name, &img);
        else
            success = AssetLoader::LoadTexture8Bit(file_name, &img, color_space == TextureColorSpace::sRGB);

        if (!success)
            return nullptr;

        return Graphics::Texture2D::Create(img.name, img.data, img.dataSize, img.format, img.width, img.height, img.mipCount, false, false);
    }

    SkinnedMeshRenderable::SkinnedMeshRenderable(Mesh* mesh, Material* material, List<Transform*> nodes, List<LoadedModel::SkinBone> bones, float max_skinning_distance)
        : m_Mesh(nullptr)
        , m_Material(nullptr)
        , m_SkinningDistance(max_skinning_distance)
        , m_Transform(nullptr)
        , m_SkinnedPrimaryBuffer(nullptr)
        , m_SkinnedSecondaryBuffer(nullptr)
        , m_BoneMatrixBuffer(nullptr)
        , m_SkeletonNodes(nodes)
        , m_Bones(bones)
    {
        if (!mesh->HasSkinningData())
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Mesh does not have skinning data, not compatible with SkinnedMeshRenderable");
            return;
        }

        m_Mesh = mesh;
        m_Material = material;

        const auto& primary_buffer = mesh->GetVertexPack().primaryBuffer;
        const auto& secondary_buffer = mesh->GetVertexPack().secondaryBuffer;

        char skinned_prim_name[100];
        sprintf(skinned_prim_name, "%s skinned", primary_buffer->GetName());

        char skinned_sec_name[100];
        sprintf(skinned_sec_name, "%s skinned", secondary_buffer->GetName());

        m_SkinnedPrimaryBuffer = Graphics::VertexBuffer::Create(skinned_prim_name, RB::Graphics::TopologyType::TriangleList, nullptr, primary_buffer->GetVertexSize(), primary_buffer->GetSize(), false, true);
        m_SkinnedSecondaryBuffer = Graphics::VertexBuffer::Create(skinned_sec_name, RB::Graphics::TopologyType::TriangleList, nullptr, secondary_buffer->GetVertexSize(), secondary_buffer->GetSize(), false, true);

        m_BoneMatrixBuffer = Graphics::GenericBuffer::Create("Bone matrices", sizeof(Math::Float4x4), bones.size(), false, true);
    }

    void SkinnedMeshRenderable::OnAttached()
    {
        m_Transform = m_GameObject->GetComponent<Transform>();
    }

    void SkinnedMeshRenderable::OnUpdate(float delta_time)
    {
        const auto& cameras = Application::GetInstance()->GetScene()->GetComponentsWithTypeOf<Camera>();

        bool in_radius = false;
        for (const auto& camera : cameras)
        {
            const Transform* t = camera->GetGameObject()->GetComponent<Transform>();
            if (t && Math::Float3::Distance(t->GetPosition(), m_Transform->GetPosition()) < m_SkinningDistance)
            {
                in_radius = true;
                break;
            }
        }

        if (!in_radius)
            return;

        Math::Float4x4* dst = (Math::Float4x4*)m_BoneMatrixBuffer->Map();

        Math::Float4x4 attach_world = m_Transform->GetLocalToWorldMatrix();
        Math::Float4x4 attach_world_inv = attach_world;
        attach_world_inv.InvertAffine();

        for (size_t i = 0; i < m_Bones.size(); i++)
        {
            const LoadedModel::SkinBone& bone = m_Bones[i];
            Transform* bone_transform = m_SkeletonNodes[bone.nodeIndex];

            // bind-space -> bone-local -> world -> back into attach-node-local space (so that gbuffer can render normally)
            dst[i] = bone.geometryToBone * bone_transform->GetLocalToWorldMatrix() * attach_world_inv;
        }
    }
}