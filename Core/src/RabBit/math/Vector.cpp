#include "RabBitCommon.h"
#include "Vector.h"

using namespace RB::Math;

// --------------------------------------------------------------------------
//									UINT 2
// --------------------------------------------------------------------------

UInt2::UInt2()
    : x(0), y(0)
{
}

UInt2::UInt2(uint32_t xy)
    : x(xy), y(xy)
{
}

UInt2::UInt2(uint32_t x, uint32_t y)
    : x(x), y(y)
{}

// --------------------------------------------------------------------------
//									UINT 4
// --------------------------------------------------------------------------

UInt4::UInt4()
    : x(0), y(0), z(0), w(0)
{
}

UInt4::UInt4(uint32_t xyzw)
    : x(xyzw), y(xyzw), z(xyzw), w(xyzw)
{
}

UInt4::UInt4(uint32_t x, uint32_t y, uint32_t z, uint32_t w)
    : x(x), y(y), z(z), w(w)
{}

// --------------------------------------------------------------------------
//									FLOAT 2
// --------------------------------------------------------------------------

Float2::Float2()
    : x(0.0f), y(0.0f)
{
}

Float2::Float2(float xy)
    : x(xy), y(xy)
{
}

Float2::Float2(float x, float y)
    : x(x), y(y)
{
}

void Float2::Normalize()
{
    *this = *this / GetLength();
}

float Float2::GetLength() const
{
    return sqrt((x * x) + (y * y));
}

Float2 Float2::operator+(const Float2& other) const
{
    return Float2(
        x + other.x,
        y + other.y
    );
}

Float2 Float2::operator+(float const& other) const
{
    return Float2(
        x + other,
        y + other
    );
}

Float2 Float2::operator-(const Float2& other) const
{
    return Float2(
        x - other.x,
        y - other.y
    );
}

Float2 Float2::operator-(const float& other) const
{
    return Float2(
        x - other,
        y - other
    );
}

Float2 Float2::operator*(const Float2& other) const
{
    return Float2(
        x * other.x,
        y * other.y
    );
}

Float2 Float2::operator*(const float& other) const
{
    return Float2(
        x * other,
        y * other
    );
}

Float2 Float2::operator/(const Float2& other) const
{
    return Float2(
        x / other.x,
        y / other.y
    );
}

Float2 Float2::operator/(const float& other) const
{
    return Float2(
        x / other,
        y / other
    );
}

float Float2::Dot(const Float2& first, const Float2& second)
{
    return (
        first.x * second.x +
        first.y * second.y
        );
}

float Float2::Angle(const Float2& first, const Float2& second)
{
    return acos(Dot(first, second) / (first.GetLength() * second.GetLength()));
}

// --------------------------------------------------------------------------
//									FLOAT 3
// --------------------------------------------------------------------------

Float3::Float3()
    : x(0.0f), y(0.0f), z(0.0f)
{
}

Float3::Float3(float xyz)
    : x(xyz), y(xyz), z(xyz)
{
}

Float3::Float3(float x, float y, float z)
    : x(x), y(y), z(z)
{
}

void Float3::Normalize()
{
    *this = *this / GetLength();
}

float Float3::GetLength() const
{
    return sqrt((x * x) + (y * y) + (z * z));
}

Float3 Float3::operator+(const Float3& other) const
{
    return Float3(
        x + other.x,
        y + other.y,
        z + other.z
    );
}

Float3 Float3::operator+(float const& other) const
{
    return Float3(
        x + other,
        y + other,
        z + other
    );
}

Float3 Float3::operator-(const Float3& other) const
{
    return Float3(
        x - other.x,
        y - other.y,
        z - other.z
    );
}

Float3 Float3::operator-(const float& other) const
{
    return Float3(
        x - other,
        y - other,
        z - other
    );
}

Float3 Float3::operator*(const Float3& other) const
{
    return Float3(
        x * other.x,
        y * other.y,
        z * other.z
    );
}

Float3 Float3::operator*(const float& other) const
{
    return Float3(
        x * other,
        y * other,
        z * other
    );
}

Float3 Float3::operator/(const Float3& other) const
{
    return Float3(
        x / other.x,
        y / other.y,
        z / other.z
    );
}

Float3 Float3::operator/(const float& other) const
{
    return Float3(
        x / other,
        y / other,
        z / other
    );
}

Float3 Float3::Cross(const Float3& first, const Float3& second)
{
    return Float3(
        first.y * second.z - first.z * second.y,
        first.z * second.x - first.x * second.z,
        first.x * second.y - first.y * second.x
    );
}

float Float3::Dot(const Float3& first, const Float3& second)
{
    return (
        first.x * second.x +
        first.y * second.y +
        first.z * second.z
        );
}

float Float3::Angle(const Float3& first, const Float3& second)
{
    return acos(Dot(first, second) / (first.GetLength() * second.GetLength()));
}

// --------------------------------------------------------------------------
//									FLOAT 4
// --------------------------------------------------------------------------

Float4::Float4()
    : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
{
}

Float4::Float4(float xyzw)
    : x(xyzw), y(xyzw), z(xyzw), w(xyzw)
{
}

Float4::Float4(float x, float y, float z, float w)
    : x(x), y(y), z(z), w(w)
{
}

Float4 Float4::operator+(const Float4& other) const
{
    return Float4(
        x + other.x,
        y + other.y,
        z + other.z,
        w + other.w
    );
}

Float4 Float4::operator+(float const& other) const
{
    return Float4(
        x + other,
        y + other,
        z + other,
        w + other
    );
}

Float4 Float4::operator*(const Float4& other) const
{
    return Float4(
        x * other.x,
        y * other.y,
        z * other.z,
        w * other.w
    );
}

Float4 Float4::operator*(const float& other) const
{
    return Float4(
        x * other,
        y * other,
        z * other,
        w * other
    );
}
