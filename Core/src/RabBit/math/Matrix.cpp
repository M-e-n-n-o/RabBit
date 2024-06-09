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
		return Float3(a03, a13, a23);
	}

	void Float4x4::SetPosition(const Float3 pos)
	{
		SetPosition(pos.x, pos.y, pos.z);
	}

	void Float4x4::SetPosition(float x, float y, float z)
	{
		a03 = x;
		a13 = y;
		a23 = z;
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