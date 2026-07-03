#pragma once
#include "RabBitCommon.h"

namespace RB::Entity
{
	// Trait to declare base classes of a concrete component type.
	template<typename T>
	struct BaseClasses
	{
		using type = std::tuple<>;
	};
}