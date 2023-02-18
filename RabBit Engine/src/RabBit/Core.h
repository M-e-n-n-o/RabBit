#pragma once

#ifdef RB_PLATFORM_WINDOWS
	#ifdef RB_BUILD_DLL
		#define RABBIT_API __declspec(dllexport)
	#else
		#define RABBIT_API __declspec(dllimport)
	#endif
#else
	#error RabBit only supports Windows!
#endif