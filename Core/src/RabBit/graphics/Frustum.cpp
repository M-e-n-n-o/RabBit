#include "RabBitCommon.h"
#include "Frustum.h"

#if defined(near)
#undef near
#endif

#if defined(far)
#undef far
#endif

namespace RB::Graphics
{
	Frustum::Frustum()
	{
		
	}

	void Frustum::SetTransform(Math::Float3 position, Math::Float3 rotation)
	{
		Math::Float4x4 m;
		m.RotateAroundX(Math::DegreesToRadians(rotation.x));
		m.RotateAroundY(Math::DegreesToRadians(rotation.y));
		m.RotateAroundZ(Math::DegreesToRadians(rotation.z));
		m.SetPosition(position);
		m.Scale(1.0f);

		SetTransform(m);
	}

	void Frustum::SetTransform(Math::Float4x4 view_to_world)
	{
		m_WorldToViewMat = view_to_world;
		m_WorldToViewMat.SetPosition(view_to_world.GetPosition() * -1);
	}

	void Frustum::SetPerspectiveProjectionVFov(float near, float far, float vfov, float aspect, bool reverse_depth)
	{
		float ty = Math::Tan(vfov * 0.5f);
		float tx = ty * aspect;

		SetPerspectiveProjection(near, far, -tx, tx, ty, -ty, reverse_depth);
	}

	void Frustum::SetPerspectiveProjection(float near, float far, float left, float right, float top, float bottom, bool reverse_depth)
	{
		if (far > kFarClipMax)
		{
			far = kFarClipMax;
		}

		if (near <= 0.0f)
		{
			near = far / (kFarClipMax * 10.0f);
		}

		m_ViewToClipMat	= Math::Float4x4();
		m_ViewToClipMat.row0 = Math::Float4(2.0f / (right - left),				0,									0,								0);
		m_ViewToClipMat.row1 = Math::Float4(0,									2.0f / (top - bottom),				0,								0);
		m_ViewToClipMat.row2 = Math::Float4((right + left) / (left - right),	(top + bottom) / (bottom - top),	far / (far - near),				1);
		m_ViewToClipMat.row3 = Math::Float4(0,									0,									(far * near) / (near - far),	0);

		if (reverse_depth)
		{
			m_ViewToClipMat.a22 = near / (near - far);
			m_ViewToClipMat.a32 = (near * far) / (far - near);
		}

		// Horizontal fov in radians
		m_HFov = Math::ArcTan2(right, 1.0f) - Math::ArcTan2(left, 1.0f);

		// Vertical fov in radians
		m_VFov = Math::ArcTan2(bottom, 1.0f) - Math::ArcTan2(top, 1.0f);

		m_AspectRatio = (right - left) / (bottom - top);

		m_ViewLength = far - near;
	}
	
	void Frustum::SetOrthographicProjection(float near, float far, float left, float right, float top, float bottom, bool reverse_depth)
	{
		if (far > kFarClipMax)
		{
			far = kFarClipMax;
		}

		if (near <= 0.0f)
		{
			near = far / (kFarClipMax * 10.0f);
		}

		m_ViewToClipMat = Math::Float4x4();
		m_ViewToClipMat.row0 = Math::Float4(2.0f / (right - left),				0,									0,						0);
		m_ViewToClipMat.row1 = Math::Float4(0,									2.0f / (top - bottom),				0,						0);
		m_ViewToClipMat.row2 = Math::Float4(0,									0,									1 / (far - near),		0);
		m_ViewToClipMat.row3 = Math::Float4((right + left) / (left - right),	(top + bottom) / (bottom - top),	near / (near - far),	1);

		if (reverse_depth)
		{
			m_ViewToClipMat.a22 = 1 / (near - far);
			m_ViewToClipMat.a32 = far / (far - near);
		}

		m_HFov = 0.0f;
		m_VFov = 0.0f;

		m_AspectRatio = (right - left) / (bottom - top);
		m_ViewLength = far - near;
	}
}