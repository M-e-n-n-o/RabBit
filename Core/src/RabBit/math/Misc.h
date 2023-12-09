#pragma once

#include "Core.h"

namespace RB::Math
{
	template<typename T>
	T Abs(T value);

	template<typename T>
	T Max(T a, T b);

	template <typename T>
	inline T AlignUpWithMask(T value, size_t mask);

	template <typename T>
	inline T AlignDownWithMask(T value, size_t mask);

	template <typename T>
	inline T AlignUp(T value, size_t alignment);

	template <typename T>
	inline T AlignDown(T value, size_t alignment);

	template <typename T>
	inline bool IsAligned(T value, size_t alignment);
}