#pragma once

#include "Core.h"
#include "utils/debug/Log.h"

#include <exception>

namespace RB::Utils::Debug
{
	#ifdef RB_ENABLE_ASSERTS

		#define RB_ASSERT_ALWAYS(tag, ...)				{ RB_LOG_ERROR(tag, __VA_ARGS__); throw std::exception(); }
		#define RB_ASSERT_FATAL(tag, check, ...)		{ if(!(check)) { RB_LOG_ERROR(tag, __VA_ARGS__); throw std::exception(); } }
		#define RB_ASSERT_FATAL_D3D(check, ...)			{ if(FAILED(check)) { RB_LOG_ERROR(RB::LOGTAG_GRAPHICS, __VA_ARGS__); throw std::exception(); } }

	#else

		#define RB_ASSERT_ALWAYS(tag, ...)
		#define RB_ASSERT_FATAL(check, ...)
		#define RB_ASSERT_FATAL_D3D(check, ...)

	#endif

	#define RB_ASSERT_ALWAYS_RELEASE(tag, ...)			{ RB_LOG_ERROR(tag, __VA_ARGS__); throw std::exception(); }
	#define RB_ASSERT_FATAL_RELEASE(tag, check, ...)	{ if(!(check)) { RB_LOG_CRITICAL(tag, __VA_ARGS__); throw std::exception(); } };
	#define RB_ASSERT_FATAL_RELEASE_D3D(check, ...)		{ if(FAILED(check)) { RB_LOG_CRITICAL(RB::LOGTAG_GRAPHICS, __VA_ARGS__); throw std::exception(); } };
}