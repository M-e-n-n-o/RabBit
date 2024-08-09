#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "graphics/RenderResource.h"

namespace RB::Entity
{
	class Mesh
	{
	public:

		Mesh(const char* name, float* vertex_data, uint32_t elements_per_vertex, uint64_t vertex_data_count, uint16_t* index_data, uint64_t index_data_count)
			: m_VertexBuffer(nullptr)
			, m_IndexBuffer(nullptr)
		{
			m_VertexBuffer = Graphics::VertexBuffer::Create(name, RB::Graphics::TopologyType::TriangleList, vertex_data, elements_per_vertex * sizeof(float), vertex_data_count * sizeof(float));

			if (index_data_count > 0)
			{
				std::string index_name = name;
				index_name += " index";

				m_IndexBuffer = Graphics::IndexBuffer::Create(index_name.c_str(), index_data, index_data_count * sizeof(uint16_t));
			}
		}

		~Mesh()
		{
			delete m_VertexBuffer;
			delete m_IndexBuffer;
		}

		Graphics::VertexBuffer* GetVertexBuffer() const
		{
			return m_VertexBuffer;
		}

		Graphics::IndexBuffer* GetIndexBuffer() const
		{
			return m_IndexBuffer;
		}

	private:
		Graphics::VertexBuffer* m_VertexBuffer;
		Graphics::IndexBuffer* m_IndexBuffer;
	};

	class Material
	{
	public:

		Material(const char* file_name);

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
		Mesh*		m_Mesh;
		Material*	m_Material;
	};
}