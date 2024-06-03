#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "graphics/RenderResource.h"

namespace RB::Entity
{
	class Mesh : public ObjectComponent
	{
	public:
		DEFINE_COMP_TAG("Mesh");

		float m_Data[15];

		Mesh()
		{
			float data[] = {
				// Pos				Color
				-0.5f, -0.5f,		1, 0, 0,
				0, 0.5f,			0, 1, 0,
				0.5f, -0.5f,		0, 0, 1,
			};

			memcpy(m_Data, data, sizeof(data));

			m_VertexBuffer = Graphics::VertexBuffer::Create("Triangle", RB::Graphics::TopologyType::TriangleList, m_Data, sizeof(float) * 5, sizeof(m_Data));
		}

		~Mesh()
		{
			delete m_VertexBuffer;
		}

		void Update() override
		{

		}

		Graphics::VertexBuffer* GetVertexBuffer() const
		{
			return m_VertexBuffer;
		}

	private:
		Graphics::VertexBuffer* m_VertexBuffer;
	};
}