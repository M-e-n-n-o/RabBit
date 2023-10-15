#pragma once

#include "Core.h"

namespace RB::Math
{
	struct Int2
	{
	public:
		union { int32_t x, r; };
		union { int32_t y, g; };

		Int2();
		Int2(int32_t xy);
		Int2(int32_t x, int32_t y);
		~Int2() = default;

		void Normalize();

		float GetLength() const;

		Int2 operator+(const Int2& other)  const;
		Int2 operator+(const int32_t& other) const;

		Int2 operator-(const Int2& other)  const;
		Int2 operator-(const int32_t& other) const;

		Int2 operator*(const Int2& other)  const;
		Int2 operator*(const int32_t& other) const;

		Int2 operator/(const Int2& other)  const;
		Int2 operator/(const int32_t& other) const;

	public:
		static Int2 Cross(const Int2& first, const Int2& second);

		static float Dot(const Int2& first, const Int2& second);

		// Returns the angle between the two vectors in radians
		static float Angle(const Int2& first, const Int2& second);
	};

	struct Float2
	{
	public:
		union { float x, r; };
		union { float y, g; };

		Float2();
		Float2(float xy);
		Float2(float x, float y);
		~Float2() = default;

		void Normalize();

		float GetLength() const;

		Float2 operator+(const Float2& other)  const;
		Float2 operator+(const float& other) const;

		Float2 operator-(const Float2& other)  const;
		Float2 operator-(const float& other) const;

		Float2 operator*(const Float2& other)  const;
		Float2 operator*(const float& other) const;

		Float2 operator/(const Float2& other)  const;
		Float2 operator/(const float& other) const;

	public:
		//static Float2 Cross(const Float2& first, const Float2& second);

		static float Dot(const Float2& first, const Float2& second);

		// Returns the angle between the two vectors in radians
		static float Angle(const Float2& first, const Float2& second);
	};

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

	//struct Float4
	//{
	//public:
	//	union { float x, r; };
	//	union { float y, g; };
	//	union { float z, b; };
	//	union { float w, a; };

	//	Float4();
	//	Float4(float xyzw);
	//	Float4(float x, float y, float z, float w);
	//	~Float4() = default;

	//	void Normalize();

	//	float GetLength() const;

	//	Float4 operator+(const Float4& other)  const;
	//	Float4 operator+(const float& other) const;

	//	Float4 operator-(const Float4& other)  const;
	//	Float4 operator-(const float& other) const;

	//	Float4 operator*(const Float4& other)  const;
	//	Float4 operator*(const float& other) const;

	//	Float4 operator/(const Float4& other)  const;
	//	Float4 operator/(const float& other) const;

	//public:
	//	static Float4 Cross(const Float4& first, const Float4& second);

	//	static float Dot(const Float4& first, const Float4& second);

	//	// Returns the angle between the two vectors in radians
	//	static float Angle(const Float4& first, const Float4& second);
	//};
}