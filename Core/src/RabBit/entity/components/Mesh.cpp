#include "RabBitCommon.h"
#include "Mesh.h"
#include "app/AssetManager.h"

namespace RB::Entity
{
    Mesh::Mesh(const char* file_name)
    {
        LoadedMesh mesh;
        bool success = AssetManager::LoadMesh(file_name, &mesh);

        if (!success)
        {
            return;
        }

        for (int i = 0; i < mesh.models.size(); i++)
        {
            LoadedMesh::Submodel submodel = mesh.models[i];

            VertexPair pair = {};

            char vertex_name[100];
            sprintf(vertex_name, "%s vertices %d", file_name, i);

            uint32_t vertex_size = sizeof(LoadedMesh::Vertex);

            pair.vertexBuffer = Graphics::VertexBuffer::Create(vertex_name, RB::Graphics::TopologyType::TriangleList, submodel.vertices.data(), vertex_size, vertex_size * submodel.vertices.size());

            if (submodel.indices.data() > 0)
            {
                char index_name[100];
                sprintf(index_name, "%s indices %d", file_name, i);

                pair.indexBuffer = Graphics::IndexBuffer::Create(index_name, submodel.indices.data(), submodel.indices.size());
            }

            m_VertexPairs.push_back(pair);
        }
    }

    Mesh::Mesh(const char* name, float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, uint16_t* index_data, uint64_t index_data_count)
    {
        VertexPair pair = {};

        pair.vertexBuffer = Graphics::VertexBuffer::Create(name, RB::Graphics::TopologyType::TriangleList, vertex_data, elements_per_vertex * sizeof(float), vertex_data_count * sizeof(float));

        if (index_data_count > 0)
        {
            std::string index_name = name;
            index_name += " index";

            pair.indexBuffer = Graphics::IndexBuffer::Create(index_name.c_str(), index_data, index_data_count);
        }

        m_VertexPairs.push_back(pair);
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