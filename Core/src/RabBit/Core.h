#pragma once

#ifdef RB_PLATFORM_WINDOWS
	#ifdef RB_CORE_ACCESS
		//#define RABBIT_API __declspec(dllexport)
	#else
		//#define RABBIT_API __declspec(dllimport)
	#endif
	#define RABBIT_API
#else
	#error RabBit only supports Windows! Make sure to specify a supported platform in the CMakeLists.txt!
#endif