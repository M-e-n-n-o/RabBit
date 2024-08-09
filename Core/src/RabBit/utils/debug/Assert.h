#pragma once

#include "Core.h"
#include "utils/debug/Log.h"

#include <exception>

namespace RB::Utils::Debug
{
	#ifdef RB_ENABLE_ASSERTS

	#ifdef RB_CORE_ACCESS
		#define RB_ASSERT(tag, check, ...)				{ if(!(check)) { RB_LOG_WARN(tag, __VA_ARGS__); RB_DEBUG_BREAK; } }
		#define RB_ASSERT_ALWAYS(tag, ...)				{ RB_LOG_ERROR(tag, __VA_ARGS__); throw std::exception(); }
		#define RB_ASSERT_FATAL(tag, check, ...)		{ if(!(check)) { RB_LOG_ERROR(tag, __VA_ARGS__); throw std::exception(); } }
		#define RB_ASSERT_FATAL_D3D(check, ...)			{ if(FAILED(check)) { RB_LOG_ERROR(RB::LOGTAG_GRAPHICS, __VA_ARGS__); throw std::exception(); } }
	#else
		#define RB_ASSERT(check, ...)					{ if(!(check)) { RB_LOG_WARN(__VA_ARGS__); RB_DEBUG_BREAK; } }
		#define RB_ASSERT_ALWAYS(...)					{ RB_LOG_ERROR(__VA_ARGS__); throw std::exception(); }
		#define RB_ASSERT_FATAL(check, ...)				{ if(!(check)) { RB_LOG_ERROR(__VA_ARGS__); throw std::exception(); } }
		#define RB_ASSERT_FATAL_D3D(check, ...)			{ if(FAILED(check)) { RB_LOG_ERROR(__VA_ARGS__); throw std::exception(); } }
	#endif

	#else

		#define RB_ASSERT(tag, check, ...)
		#define RB_ASSERT_ALWAYS(tag, ...)
		#define RB_ASSERT_FATAL(check, ...)
		#define RB_ASSERT_FATAL_D3D(check, ...)

	#endif

#ifdef RB_CORE_ACCESS
	#define RB_ASSERT_ALWAYS_RELEASE(tag, ...)			{ RB_LOG_ERROR(tag, __VA_ARGS__); throw std::exception(); }
	#define RB_ASSERT_FATAL_RELEASE(tag, check, ...)	{ if(!(check)) { RB_LOG_CRITICAL(tag, __VA_ARGS__); throw std::exception(); } };
	#define RB_ASSERT_FATAL_RELEASE_D3D(check, ...)		{ if(FAILED(check)) { RB_LOG_CRITICAL(RB::LOGTAG_GRAPHICS, __VA_ARGS__); throw std::exception(); } };
#else
	#define RB_ASSERT_ALWAYS_RELEASE(...)				{ RB_LOG_ERROR(__VA_ARGS__); throw std::exception(); }
	#define RB_ASSERT_FATAL_RELEASE(check, ...)			{ if(!(check)) { RB_LOG_CRITICAL(__VA_ARGS__); throw std::exception(); } };
	#define RB_ASSERT_FATAL_RELEASE_D3D(check, ...)		{ if(FAILED(check)) { RB_LOG_CRITICAL(__VA_ARGS__); throw std::exception(); } };
#endif
}