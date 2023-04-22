#include "RabBitPch.h"
#include "utils/math/Vec.h"

using namespace RB::math;

Vec3::Vec3()
	: x(0.0f), y(0.0f), z(0.0f)
{
}

Vec3::Vec3(float xyz)
	: x(xyz), y(xyz), z(xyz)
{
}

Vec3::Vec3(float x, float y, float z)
	: x(x), y(y), z(z)
{
}

void Vec3::Normalize()
{
	*this = *this / GetLength();
}

float Vec3::GetLength() const
{
	return sqrt((x * x) + (y * y) + (z * z));
}

Vec3 Vec3::operator+(const Vec3& other) const
{
	return Vec3(
		x + other.x,
		y + other.y,
		z + other.z
	);
}

Vec3 Vec3::operator+(float const& other) const
{
	return Vec3(
		x + other,
		y + other,
		z + other
	);
}

Vec3 Vec3::operator-(const Vec3& other) const
{
	return Vec3(
		x - other.x,
		y - other.y,
		z - other.z
	);
}

Vec3 Vec3::operator-(const float& other) const
{
	return Vec3(
		x - other,
		y - other,
		z - other
	);
}

Vec3 Vec3::operator*(const Vec3& other) const
{
	return Vec3(
		x * other.x,
		y * other.y,
		z * other.z
	);
}

Vec3 Vec3::operator*(const float& other) const
{
	return Vec3(
		x * other,
		y * other,
		z * other
	);
}

Vec3 Vec3::operator/(const Vec3& other) const
{
	return Vec3(
		x / other.x,
		y / other.y,
		z / other.z
	);
}

Vec3 Vec3::operator/(const float& other) const
{
	return Vec3(
		x / other,
		y / other,
		z / other
	);
}

Vec3 Vec3::Cross(const Vec3& first, const Vec3& second)
{
	return Vec3(
		first.y * second.z - first.z * second.y,
		first.z * second.x - first.x * second.z,
		first.x * second.y - first.y * second.x
	);
}

float Vec3::Dot(const Vec3& first, const Vec3& second)
{
	return (
		first.x * second.x +
		first.y * second.y +
		first.z * second.z
	);
}

float Vec3::Angle(const Vec3& first, const Vec3& second)
{
	return acos(Dot(first, second) / (first.GetLength() * second.GetLength()));
}
