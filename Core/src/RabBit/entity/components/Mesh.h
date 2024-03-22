#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.cpp"

namespace RB::Entity
{
	class Mesh : public ObjectComponent
	{
	public:
		DEFINE_COMP_TAG("Mesh");

		void Update() override
		{

		}

		List<float> GetVertexData()
		{
			return List<float>({
				// Pos				Color
				-0.5f, -0.5f,		1, 0, 0,
				0, 0.5f,			0, 1, 0,
				0.5f, -0.5f,		0, 0, 1,
			});
		}
	};
}