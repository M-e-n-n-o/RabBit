#include "RabBitCommon.h"
#include "Misc.h"

namespace RB::Math
{
	float Abs(float value)
	{
		return value > 0 ? value : -value;
	}
}