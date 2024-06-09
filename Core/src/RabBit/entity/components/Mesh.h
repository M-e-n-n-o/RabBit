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
			: m_VertexData(nullptr)
			, m_IndexData(nullptr)
			, m_VertexBuffer(nullptr)
			, m_IndexBuffer(nullptr)
		{
			m_IsUploaded = false;

			uint64_t vertex_data_size = vertex_data_count * sizeof(float);
			m_VertexData = (float*) ALLOC_HEAP(vertex_data_size);
			memcpy(m_VertexData, vertex_data, vertex_data_size);

			m_VertexBuffer = Graphics::VertexBuffer::Create(name, RB::Graphics::TopologyType::TriangleList, m_VertexData, sizeof(float) * elements_per_vertex, vertex_data_size);

			if (index_data_count > 0)
			{
				uint64_t index_data_size = index_data_count * sizeof(uint16_t);
				m_IndexData = (uint16_t*) ALLOC_HEAP(index_data_size);
				memcpy(m_IndexData, index_data, index_data_size);

				std::string index_name = name;
				index_name += " index";

				m_IndexBuffer = Graphics::IndexBuffer::Create(index_name.c_str(), m_IndexData, index_data_size);
			}
		}

		~Mesh()
		{
			SAFE_FREE(m_VertexData);
			SAFE_FREE(m_IndexData);
			delete m_VertexBuffer;
			delete m_IndexBuffer;
		}

		// Creates an upload resource the size of range which will stay alive until writable is set to false
		//void SetWritable(bool writable, Math::Float2 range);
		//void BeginEdit()
		//void EndEdit()

		bool IsLatestDataUploaded() const {	return m_IsUploaded; }
		void SetLatestDataUploaded(bool uploaded) 
		{ 
			m_IsUploaded = uploaded; 

			if (m_IsUploaded)
			{
				// TODO Enable deleting the data after it has been uploaded again when this method is called at the proper time
				//SAFE_FREE(m_VertexData);
				//SAFE_FREE(m_IndexData);
			}
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
		float* m_VertexData;
		uint16_t* m_IndexData;

		bool m_IsUploaded;

		Graphics::VertexBuffer* m_VertexBuffer;
		Graphics::IndexBuffer* m_IndexBuffer;
	};

	class MeshRenderer : public ObjectComponent
	{
	public:
		DEFINE_COMP_TAG("MeshRenderer");

		MeshRenderer(Mesh* mesh)
		{
			m_Mesh = mesh;
		}

		Mesh* GetMesh() const
		{
			return m_Mesh;
		}

	private:
		Mesh* m_Mesh;
	};
}