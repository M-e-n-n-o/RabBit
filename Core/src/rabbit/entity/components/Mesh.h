#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "graphics/RenderResource.h"
#include "app/AssetLoader.h"

namespace RB::Entity
{
    class Transform;

    class Mesh
    {
    public:
        struct VertexPack
        {
            // The vertex buffers are split up in 2
            // - The first one atleast contains all the position data (for shadow rendering)
            // - The secondary one all the optional data (such as normals, UV, etc.)
            Shared<Graphics::VertexBuffer> primaryBuffer    = nullptr;
            Shared<Graphics::VertexBuffer> secondaryBuffer  = nullptr;
            Shared<Graphics::IndexBuffer>  indexBuffer      = nullptr;

            // For skinned meshes only
            Shared<Graphics::VertexBuffer> boneWeightBuffer = nullptr;
        };

        Mesh(const char* name, const LoadedModel::Submodel& submodel, bool allow_skinning = true);
        Mesh(const char* name, const float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, const uint32_t* index_data = nullptr, uint64_t index_data_count = 0);

        const VertexPack& GetVertexPack() const { return m_VertexPack; }

        bool HasValidAABB() const { return m_ValidBounds; }
        const Math::AABB& GetAABB() const { return m_Bounds; }

        bool HasSkinningData() const { return m_VertexPack.boneWeightBuffer != nullptr; }

    private:
        VertexPack m_VertexPack;

        Math::AABB m_Bounds;
        bool       m_ValidBounds;
    };

    enum class TextureColorSpace
    {
        Linear,
        sRGB
    };

    class Material
    {
    public:

        Material();
        Material(const char* name, LoadedImage* image);
        Material(const char* file_name, bool converted_texture, TextureColorSpace color_space = TextureColorSpace::sRGB);

        Shared<Graphics::Texture2D> GetTexture() const { return m_Texture; }

    private:
        Shared<Graphics::Texture2D> m_Texture;
    };

    class MeshRenderable : public ObjectComponent
    {
    public:
        MeshRenderable(Mesh* mesh, Material* material)
        {
            m_Mesh = mesh;
            m_Material = material;
        }

        Mesh* GetMesh() const { return m_Mesh; }
        Material* GetMaterial() const { return m_Material; }

    private:
        Mesh* m_Mesh;
        Material* m_Material;
    };

    class SkinnedMeshRenderable : public ObjectComponent
    {
    public:
        SkinnedMeshRenderable(Mesh* mesh, Material* material, List<Transform*> nodes, List<LoadedModel::SkinBone> bones, float max_skinning_distance = 100.0f);

        void OnAttached() override;

        void OnUpdate(float delta_time) override;

        Mesh* GetMesh() const { return m_Mesh; }
        Material* GetMaterial() const { return m_Material; }
        float GetSkinningDistance() const { return m_SkinningDistance; }

        Shared<Graphics::VertexBuffer> GetSkinnedPrimaryBuffer() const { return m_SkinnedPrimaryBuffer; }
        Shared<Graphics::VertexBuffer> GetSkinnedSecondaryBuffer() const { return m_SkinnedSecondaryBuffer; }
        Shared<Graphics::GenericBuffer> GetBoneMatrixBuffer() const { return m_BoneMatrixBuffer; }

    private:
        Mesh*                           m_Mesh;
        Material*                       m_Material;
        float                           m_SkinningDistance;

        Transform*                      m_Transform;
        Shared<Graphics::VertexBuffer>  m_SkinnedPrimaryBuffer;
        Shared<Graphics::VertexBuffer>  m_SkinnedSecondaryBuffer;
        Shared<Graphics::GenericBuffer> m_BoneMatrixBuffer;
        List<Transform*>                m_SkeletonNodes;
        List<LoadedModel::SkinBone>     m_Bones;
    };
}