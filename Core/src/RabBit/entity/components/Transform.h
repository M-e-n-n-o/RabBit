#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"

namespace RB::Entity
{
	class Transform : public ObjectComponent
	{
	public:
		DEFINE_COMP_TAG("Transform");

		Math::Float3 position;
		Math::Float3 rotation;
		Math::Float3 scale;

		// Transformation matrix
		Math::Float4x4 GetLocalToWorldMatrix() const
		{
			Math::Float4x4 m;
			m.SetPosition(position);
			m.Rotate(rotation);
			m.Scale(scale);

			return m;
		}
	};
}