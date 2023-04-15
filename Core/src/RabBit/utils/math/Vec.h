#pragma once

#include "Core.h"

namespace RB::math
{
	struct Vec3
	{
	public:
		union { float x, r; };
		union { float y, g; };
		union { float z, b; };

		Vec3();
		Vec3(float xyz);
		Vec3(float x, float y, float z);
	    ~Vec3() = default;

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

	struct Vec4
	{
	public:
		union { float x, r; };
		union { float y, g; };
		union { float z, b; };
		union { float w, a; };

		Vec4();
		Vec4(float xyzw);
		Vec4(float x, float y, float z, float w);
		~Vec4() = default;

		void Normalize();

		float GetLength() const;

		Vec4 operator+(const Vec4& other)  const;
		Vec4 operator+(const float& other) const;

		Vec4 operator-(const Vec4& other)  const;
		Vec4 operator-(const float& other) const;

		Vec4 operator*(const Vec4& other)  const;
		Vec4 operator*(const float& other) const;

		Vec4 operator/(const Vec4& other)  const;
		Vec4 operator/(const float& other) const;

	public:
		static Vec4 Cross(const Vec4& first, const Vec4& second);

		static float Dot(const Vec4& first, const Vec4& second);

		// Returns the angle between the two vectors in radians
		static float Angle(const Vec4& first, const Vec4& second);
	};
}