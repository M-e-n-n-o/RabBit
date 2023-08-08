#include "RabBitCommon.h"
#include "Vector.h"

using namespace RB::Math;

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
