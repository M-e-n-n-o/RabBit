#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>

namespace RB
{
	template<class T>
	using GPtr = Microsoft::WRL::ComPtr<T>;
}