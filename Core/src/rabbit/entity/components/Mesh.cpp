#include "RabBitCommon.h"
#include "Mesh.h"
#include "app/AssetManager.h"
#include "graphics/ResourceDefaults.h"

namespace RB::Entity
{
    Mesh::Mesh(const char* name, LoadedMesh::Submodel& submodel)
    {
        char position_name[100];
        sprintf(position_name, "%s primary", name);

        char secondary_name[100];
        sprintf(secondary_name, "%s secondary", name);

        uint32_t position_size = sizeof(Math::Float3);
        uint32_t vertex_size = sizeof(LoadedMesh::Vertex);

        m_VertexPack.primaryBuffer   = Graphics::VertexBuffer::Create(position_name,  RB::Graphics::TopologyType::TriangleList, submodel.positions.data(), position_size, position_size * submodel.positions.size());
        m_VertexPack.secondaryBuffer = Graphics::VertexBuffer::Create(secondary_name, RB::Graphics::TopologyType::TriangleList, submodel.vertices.data(), vertex_size, vertex_size * submodel.vertices.size());

        if (!submodel.indices.empty())
        {
            char index_name[100];
            sprintf(index_name, "%s indices", name);

            m_VertexPack.indexBuffer = Graphics::IndexBuffer::Create(index_name, submodel.indices.data(), submodel.indices.size());
        }

        m_ValidBounds = true;
        m_Bounds.min = submodel.minBounds;
        m_Bounds.max = submodel.maxBounds;
    }

    Mesh::Mesh(const char* name, float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, uint32_t* index_data, uint64_t index_data_count)
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
        m_Texture = Graphics::g_TexDefaultError;
    }

    Material::Material(const char* name, LoadedImage* image)
    {
        m_Texture = Graphics::Texture2D::Create(name, image->data, image->dataSize, image->format, image->width, image->height, false, false);
    }

    Material::Material(const char* file_name, bool converted_texture, TextureColorSpace color_space)
        : m_Texture(nullptr)
    {
        LoadedImage img;

        bool success;
        if (converted_texture)
            success = AssetManager::LoadConvertedTexture(file_name, &img);
        else
            success = AssetManager::LoadTexture8Bit(file_name, &img, color_space == TextureColorSpace::sRGB);

        if (success)
        {
            m_Texture = Graphics::Texture2D::Create(img.name, img.data, img.dataSize, img.format, img.width, img.height, false, false);
        }
        else
        {
            m_Texture = Graphics::g_TexDefaultError;
        }
    }
}