#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "graphics/RenderResource.h"

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

        Mesh(const char* file_name);
        Mesh(const char* name, float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, uint16_t* index_data, uint64_t index_data_count);

        ~Mesh()
        {
            for (int i = 0; i < m_VertexPairs.size(); i++)
            {
                SAFE_DELETE(m_VertexPairs[i].vertexBuffer);
                SAFE_DELETE(m_VertexPairs[i].indexBuffer);
            }
        }

        const List<VertexPair>& GetVertexPairs() const
        {
            return m_VertexPairs;
        }

    private:
        List<VertexPair> m_VertexPairs;
    };

    class Material
    {
    public:

        Material(const char* file_name, Graphics::TextureColorSpace color_space = Graphics::TextureColorSpace::sRGB);

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