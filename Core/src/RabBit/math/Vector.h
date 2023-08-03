#pragma once

#include "Core.h"

namespace RB::Math
{
	struct Float3
	{
	public:
		union { float x, r; };
		union { float y, g; };
		union { float z, b; };

		Float3();
		Float3(float xyz);
		Float3(float x, float y, float z);
	    ~Float3() = default;

		void Normalize();

		float GetLength() const;

		Float3 operator+(const Float3& other)  const;
		Float3 operator+(const float& other) const;

		Float3 operator-(const Float3& other)  const;
		Float3 operator-(const float& other) const;

		Float3 operator*(const Float3& other)  const;
		Float3 operator*(const float& other) const;

		Float3 operator/(const Float3& other)  const;
		Float3 operator/(const float& other) const;

	public:
		static Float3 Cross(const Float3& first, const Float3& second);

		static float Dot(const Float3& first, const Float3& second);

		// Returns the angle between the two vectors in radians
		static float Angle(const Float3& first, const Float3& second);
	};

	struct Float4
	{
	public:
		union { float x, r; };
		union { float y, g; };
		union { float z, b; };
		union { float w, a; };

		Float4();
		Float4(float xyzw);
		Float4(float x, float y, float z, float w);
		~Float4() = default;

		void Normalize();

		float GetLength() const;

		Float4 operator+(const Float4& other)  const;
		Float4 operator+(const float& other) const;

		Float4 operator-(const Float4& other)  const;
		Float4 operator-(const float& other) const;

		Float4 operator*(const Float4& other)  const;
		Float4 operator*(const float& other) const;

		Float4 operator/(const Float4& other)  const;
		Float4 operator/(const float& other) const;

	public:
		static Float4 Cross(const Float4& first, const Float4& second);

		static float Dot(const Float4& first, const Float4& second);

		// Returns the angle between the two vectors in radians
		static float Angle(const Float4& first, const Float4& second);
	};
}