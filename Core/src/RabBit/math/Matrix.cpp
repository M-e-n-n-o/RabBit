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