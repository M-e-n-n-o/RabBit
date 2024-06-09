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
		Camera(float near_plane, float far_plane, float vfov, uint32_t target_window_index)
			: m_Near(near_plane)
			, m_Far(far_plane)
			, m_VFovDegrees(vfov)
			, m_TargetWindowIndex(target_window_index)
		{
		}

		uint32_t GetTargetWindowIndex()	const { return m_TargetWindowIndex; }

		float GetNearPlane()			const { return m_Near; }
		float GetFarPlane()				const { return m_Far; }
		float GetVerticalFovInDegrees()	const { return m_VFovDegrees; }
		float GetVerticalFovInRadians()	const { return Math::DegreesToRadians(m_VFovDegrees); }

	private:
		float		m_Near;
		float		m_Far;
		float		m_VFovDegrees;

		uint32_t	m_TargetWindowIndex;
	};
}