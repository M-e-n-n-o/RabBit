#pragma once

#include "Core.h"
#include "utils/debug/Log.h"

#include <exception>

namespace RB
{
	#ifdef RB_ENABLE_ASSERTS

		#define RB_ASSERT_FATAL(x, ...)		{ if(!(x)) { RB_LOG_ERROR(__VA_ARGS__); throw std::exception(); } }
		#define RB_ASSERT_FATAL_D3D(x, ...)	{ if(FAILED(x)) { RB_LOG_ERROR(__VA_ARGS__); throw std::exception(); } }

	#else

		#define RB_ASSERT_FATAL(x, ...)
		#define RB_ASSERT_FATAL_D3D(x, ...)

	#endif

	#define RB_ASSERT_FATAL_RELEASE(x, ...)		{ if(!(x)) { RB_LOG_CRITICAL(__VA_ARGS__); throw std::exception(); } };
	#define RB_ASSERT_FATAL_RELEASE_D3D(x, ...)	{ if(FAILED(x)) { RB_LOG_CRITICAL(__VA_ARGS__); throw std::exception(); } };
}