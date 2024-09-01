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

        static Int2 Cross(const Int2& first, const Int2& second);

        static float Dot(const Int2& first, const Int2& second);

        // Returns the angle between the two vectors in radians
        static float Angle(const Int2& first, const Int2& second);
    };

    struct Int4
    {
    public:
        union
        {
            struct
            {
                int32_t arr[4];
            };

            struct
            {
                int32_t x;
                int32_t y;
                int32_t z;
                int32_t w;
            };

            struct
            {
                int32_t r;
                int32_t g;
                int32_t b;
                int32_t a;
            };
        };

        Int4();
        Int4(int32_t xyzw);
        Int4(int32_t x, int32_t y, int32_t z, int32_t w);
        ~Int4() = default;

        void Normalize();

        float GetLength() const;

        Int4 operator+(const Int4& other)  const;
        Int4 operator+(const int32_t& other) const;

        Int4 operator-(const Int4& other)  const;
        Int4 operator-(const int32_t& other) const;

        Int4 operator*(const Int4& other)  const;
        Int4 operator*(const int32_t& other) const;

        Int4 operator/(const Int4& other)  const;
        Int4 operator/(const int32_t& other) const;

        static Int4 Cross(const Int4& first, const Int4& second);

        static int32_t Dot(const Int4& first, const Int4& second);

        // Returns the angle between the two vectors in radians
        static int32_t Angle(const Int4& first, const Int4& second);
    };

    struct UInt4
    {
    public:
        union
        {
            struct
            {
                uint32_t arr[4];
            };

            struct
            {
                uint32_t x;
                uint32_t y;
                uint32_t z;
                uint32_t w;
            };

            struct
            {
                uint32_t r;
                uint32_t g;
                uint32_t b;
                uint32_t a;
            };
        };

        UInt4();
        UInt4(uint32_t xyzw);
        UInt4(uint32_t x, uint32_t y, uint32_t z, uint32_t w);
        ~UInt4() = default;

        void Normalize();

        float GetLength() const;

        UInt4 operator+(const UInt4& other)  const;
        UInt4 operator+(const uint32_t& other) const;

        UInt4 operator-(const UInt4& other)  const;
        UInt4 operator-(const uint32_t& other) const;

        UInt4 operator*(const UInt4& other)  const;
        UInt4 operator*(const uint32_t& other) const;

        UInt4 operator/(const UInt4& other)  const;
        UInt4 operator/(const uint32_t& other) const;

        static UInt4 Cross(const UInt4& first, const UInt4& second);

        static uint32_t Dot(const UInt4& first, const UInt4& second);

        // Returns the angle between the two vectors in radians
        static uint32_t Angle(const UInt4& first, const UInt4& second);
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

        //	void Normalize();

        //	float GetLength() const;

        Float4 operator+(const Float4& other)  const;
        Float4 operator+(const float& other) const;

        //	Float4 operator-(const Float4& other)  const;
        //	Float4 operator-(const float& other) const;

        Float4 operator*(const Float4& other)  const;
        Float4 operator*(const float& other) const;

        //	Float4 operator/(const Float4& other)  const;
        //	Float4 operator/(const float& other) const;

        //	static Float4 Cross(const Float4& first, const Float4& second);

        //	static float Dot(const Float4& first, const Float4& second);

        //	// Returns the angle between the two vectors in radians
        //	static float Angle(const Float4& first, const Float4& second);
    };
}