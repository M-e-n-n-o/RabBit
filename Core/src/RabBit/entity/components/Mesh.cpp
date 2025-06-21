#include "RabBitCommon.h"
#include "Mesh.h"
#include "app/AssetManager.h"

namespace RB::Entity
{
    Mesh::Mesh(const char* file_name)
        : m_VertexBuffer(nullptr)
        , m_IndexBuffer(nullptr)
    {
        LoadedModel model;
        bool success = AssetManager::LoadModel(file_name, &model);

        if (success)
        {
            std::string vertex_name = file_name;
            vertex_name += " vertices";

            uint32_t vertex_size = sizeof(LoadedModel::Vertex);

            m_VertexBuffer = Graphics::VertexBuffer::Create(vertex_name.c_str(), RB::Graphics::TopologyType::TriangleList, model.vertices.data(), vertex_size, vertex_size * model.vertices.size());

            if (model.indices.data() > 0)
            {
                std::string index_name = file_name;
                index_name += " indices";

                m_IndexBuffer = Graphics::IndexBuffer::Create(index_name.c_str(), model.indices.data(), model.indices.size());
            }
        }
    }

    Mesh::Mesh(const char* name, float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, uint16_t* index_data, uint64_t index_data_count)
        : m_VertexBuffer(nullptr)
        , m_IndexBuffer(nullptr)
    {
        m_VertexBuffer = Graphics::VertexBuffer::Create(name, RB::Graphics::TopologyType::TriangleList, vertex_data, elements_per_vertex * sizeof(float), vertex_data_count * sizeof(float));

        if (index_data_count > 0)
        {
            std::string index_name = name;
            index_name += " index";

            m_IndexBuffer = Graphics::IndexBuffer::Create(index_name.c_str(), index_data, index_data_count);
        }
    }

    Material::Material(const char* file_name, Graphics::TextureColorSpace color_space)
        : m_Texture(nullptr)
    {
        LoadedImage img;
        bool success = AssetManager::LoadImage8Bit(file_name, &img);

        if (success)
        {
            m_Texture = Graphics::Texture2D::Create(file_name, img.data, img.dataSize, img.format, img.width, img.height, false, false, color_space);
        }
    }
}