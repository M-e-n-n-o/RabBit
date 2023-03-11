#pragma once

#include "Core.h"

namespace RB::math
{
	class RABBIT_API Vec3
	{
	public:
		Vec3();
		Vec3(float xyz);
		Vec3(float x, float y, float z);
	    ~Vec3() = default;

		union { float x, r; };
		union { float y, g; };
		union { float z, b; };

		void Normalize();

		float GetLength() const;

		Vec3 operator+(const Vec3& other)  const;
		Vec3 operator+(const float& other) const;

		Vec3 operator-(const Vec3& other)  const;
		Vec3 operator-(const float& other) const;

		Vec3 operator*(const Vec3& other)  const;
		Vec3 operator*(const float& other) const;

		Vec3 operator/(const Vec3& other)  const;
		Vec3 operator/(const float& other) const;

	public:
		static Vec3 Cross(const Vec3& first, const Vec3& second);

		static float Dot(const Vec3& first, const Vec3& second);

		// Returns the angle between the two vectors in radians
		static float Angle(const Vec3& first, const Vec3& second);
	};
}