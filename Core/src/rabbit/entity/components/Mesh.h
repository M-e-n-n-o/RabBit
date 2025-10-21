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
        struct VertexPair
        {
            Graphics::VertexBuffer* vertexBuffer = nullptr;
            Graphics::IndexBuffer*  indexBuffer = nullptr;
        };

        Mesh(const char* name, LoadedMesh::Submodel& submodel);
        Mesh(const char* name, float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, uint16_t* index_data, uint64_t index_data_count);

        ~Mesh()
        {
            SAFE_DELETE(m_VertexPair.vertexBuffer);
            SAFE_DELETE(m_VertexPair.indexBuffer);
        }

        const VertexPair& GetVertexPair() const
        {
            return m_VertexPair;
        }

    private:
        VertexPair m_VertexPair;
    };

    enum class TextureColorSpace
    {
        Linear,
        sRGB
    };

    class Material
    {
    public:

        Material(const char* file_name, TextureColorSpace color_space = TextureColorSpace::sRGB);

        ~Material()
        {
            delete m_Texture;
        }

        Graphics::Texture2D* GetTexture() const
        {
            return m_Texture;
        }

    private:
        Graphics::Texture2D* m_Texture;
    };

    class MeshRenderer : public ObjectComponent
    {
    public:
        DEFINE_COMP_TAG("MeshRenderer");

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