#pragma once

#ifdef _WIN32
	#define RB_PLATFORM_WINDOWS

	#ifdef RB_CORE_ACCESS
		//#define RABBIT_API __declspec(dllexport)
	#else
		//#define RABBIT_API __declspec(dllimport)
	#endif

	#ifdef RB_CONFIG_DEBUG
		#define RB_ENABLE_ASSERTS
		#define RB_ENABLE_LOGS
	#endif

	#ifdef RB_CONFIG_RELEASE
		#define RB_ENABLE_ASSERTS
		#define RB_ENABLE_LOGS
	#endif

	#ifdef RB_CONFIG_DIST
	#endif

#else
	#error Make sure to specify a supported platform in the CMakeLists.txt! RabBit only supports Windows!
#endif
