#pragma once

#include "math/Matrix.h"
#include "math/Vector.h"

namespace RB::Graphics
{
	// Precision issues with high far-clip values.
	// It should be possible to increase this limit to 65504 if needed
	// (FP16 linear depth cannot encode above 65504).
	#define kFarClipMax 32767.0f

	class Frustum
	{
	public:
		Frustum();

		void SetTransform(Math::Float3 position, Math::Float3 rotation);
		void SetTransform(Math::Float4x4 world_position);

		// Projection matrix
		Math::Float4x4 GetViewToClipMatrix() const { return m_ViewToClipMat; }

		void SetPerspectiveProjectionVFov(float near, float far, float vfov, float aspect, bool reverse_depth);
		void SetPerspectiveProjection(float near, float far, float left, float right, float top, float bottom, bool reverse_depth);

	private:
		Math::Float4x4	m_WorldToViewMat;	// World space to view space matrix
		Math::Float4x4	m_ViewToClipMat;	// View space to clip space matrix (projection matrix)

		float			m_VFov;
		float			m_HFov;
		float			m_AspectRatio;
		float			m_ViewLength;
	};
}