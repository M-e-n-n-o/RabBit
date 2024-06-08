#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "graphics/RenderResource.h"

namespace RB::Entity
{
	class Mesh
	{
	public:

		Mesh()
		{
			m_IsUploaded = false;

			float data[] = {
				// Pos				Color
				-0.5f, -0.5f,		1, 0, 0,
				0, 0.5f,			0, 1, 0,
				0.5f, -0.5f,		0, 0, 1,
			};

			uint64_t size = 15 * sizeof(float);

			m_Data = (float*) ALLOC_HEAP(size);
			memcpy(m_Data, data, size);

			m_VertexBuffer = Graphics::VertexBuffer::Create("Triangle", RB::Graphics::TopologyType::TriangleList, m_Data, sizeof(float) * 5, size);
		}

		~Mesh()
		{
			SAFE_FREE(m_Data);
			delete m_VertexBuffer;
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
				//SAFE_FREE(m_Data);
			}
		}

		Graphics::VertexBuffer* GetVertexBuffer() const
		{
			return m_VertexBuffer;
		}

	private:
		float* m_Data;

		bool m_IsUploaded;

		Graphics::VertexBuffer* m_VertexBuffer;
	};

	class MeshRenderer : public ObjectComponent
	{
	public:
		DEFINE_COMP_TAG("MeshRenderer");

		MeshRenderer()
		{
			m_Mesh = new Mesh();
		}

		~MeshRenderer()
		{
			delete m_Mesh;
		}

		void Update() override
		{

		}

		Mesh* GetMesh() const
		{
			return m_Mesh;
		}

	private:
		Mesh* m_Mesh;
	};
}