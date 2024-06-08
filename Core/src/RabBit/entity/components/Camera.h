#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"

namespace RB::Entity
{
	class Camera : public ObjectComponent
	{
	public:
		DEFINE_COMP_TAG("Camera");

		// FOV in degrees
		Camera(float near, float far, float vfov)
			: m_Near(near)
			, m_Far(far)
			, m_VFovDegrees(vfov)
		{
		}

		float GetNearPlane()			const { return m_Near; }
		float GetFarPlane()				const { return m_Far; }
		float GetVerticalFovInDegrees()	const { return m_VFovDegrees; }
		float GetVerticalFovInRadians()	const { return Math::DegreesToRadians(m_VFovDegrees); }

	private:
		float m_Near;
		float m_Far;
		float m_VFovDegrees;
	};
}