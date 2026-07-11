#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "graphics/RenderResource.h"
#include "app/AssetManager.h"

namespace RB::Entity
{
    class Mesh
    {
    public:
        struct VertexPack
        {
            // The vertex buffers are split up in 2
            // - The first one atleast contains all the position data (for shadow rendering)
            // - The secondary one all the optional data (such as normals, UV, etc.)
            Shared<Graphics::VertexBuffer> primaryBuffer = nullptr;
            Shared<Graphics::VertexBuffer> secondaryBuffer = nullptr;
            Shared<Graphics::IndexBuffer>  indexBuffer = nullptr;
        };

        Mesh(const char* name, LoadedMesh::Submodel& submodel);
        Mesh(const char* name, float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, uint32_t* index_data = nullptr, uint64_t index_data_count = 0);

        const VertexPack& GetVertexPack() const
        {
            return m_VertexPack;
        }

        bool HasValidAABB() const { return m_ValidBounds; }
        const Math::AABB& GetAABB() const { return m_Bounds; }

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

        Shared<Graphics::Texture2D> GetTexture() const
        {
            return m_Texture;
        }

    private:
        Shared<Graphics::Texture2D> m_Texture;
    };

    class MeshRenderer : public ObjectComponent
    {
    public:
        MeshRenderer(Mesh* mesh, Material* material)
        {
            m_Mesh = mesh;
            m_Material = material;
        }

        Mesh* GetMesh() const
        {
            return m_Mesh;
        }

        Material* GetMaterial() const
        {
            return m_Material;
        }

    private:
        Mesh* m_Mesh;
        Material* m_Material;
    };
}