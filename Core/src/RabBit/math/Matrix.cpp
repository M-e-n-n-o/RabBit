#include "RabBitCommon.h"
#include "Matrix.h"

namespace RB::Math
{
	Float4x4::Float4x4()
	{
		row0 = { 1, 0, 0, 0 };
		row1 = { 0, 1, 0, 0 };
		row2 = { 0, 0, 1, 0 };
		row3 = { 0, 0, 0, 1 };
	}

	void Float4x4::ToData(float* out)
	{
		memcpy(out, a, 16 * sizeof(float));
	}

	Float4x4 Float4x4::operator*(const Float4x4& other)
	{
		Float4x4 out;
		out.row0 = (other.row0 * row0.x) + (other.row1 * row0.y) + (other.row2 * row0.z) + (other.row3 * row0.w);
		out.row1 = (other.row0 * row1.x) + (other.row1 * row1.y) + (other.row2 * row1.z) + (other.row3 * row1.w);
		out.row2 = (other.row0 * row2.x) + (other.row1 * row2.y) + (other.row2 * row2.z) + (other.row3 * row2.w);
		out.row3 = (other.row0 * row3.x) + (other.row1 * row3.y) + (other.row2 * row3.z) + (other.row3 * row3.w);

		return out;
	}

	void Float4x4::Transpose()
	{
		Float4x4 copy = *this;

		row0 = { copy.row0.x, copy.row1.x, copy.row2.x, copy.row3.x };
		row1 = { copy.row0.y, copy.row1.y, copy.row2.y, copy.row3.y };
		row2 = { copy.row0.z, copy.row1.z, copy.row2.z, copy.row3.z };
		row3 = { copy.row0.w, copy.row1.w, copy.row2.w, copy.row3.w };
	}

	void Float4x4::Rotate(const Float3 euler_angles)
	{
		// TODO
	}

	Float3 Float4x4::GetPosition()
	{
		return Float3(a30, a31, a32);
	}

	void Float4x4::SetPosition(const Float3 pos)
	{
		SetPosition(pos.x, pos.y, pos.z);
	}

	void Float4x4::SetPosition(float x, float y, float z)
	{
		a30 = x;
		a31 = y;
		a32 = z;
		a33 = 1;
	}

	void Float4x4::Scale(const Float3 scale)
	{
		Scale(scale.x, scale.y, scale.z);
	}

	void Float4x4::Scale(float scale)
	{
		Scale(scale, scale, scale);
	}

	void Float4x4::Scale(float x, float y, float z)
	{
		a00 *= x;
		a11 *= y;
		a22 *= z;
	}
}