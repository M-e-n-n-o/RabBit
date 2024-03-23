#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.cpp"
#include "graphics/RenderResource.h"

namespace RB::Entity
{
	class Mesh : public ObjectComponent
	{
	public:
		DEFINE_COMP_TAG("Mesh");

		Mesh()
		{
			float data[] =
			{
				// Pos				Color
				-0.5f, -0.5f,		1, 0, 0,
				0, 0.5f,			0, 1, 0,
				0.5f, -0.5f,		0, 0, 1,
			};

			m_VertexBuffer = Graphics::VertexBuffer::Create("Triangle", data, sizeof(data));
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