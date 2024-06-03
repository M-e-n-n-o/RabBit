#pragma once

#include "Core.h"

#ifndef RB_ENABLE_LOGS
	#include <cstdio>
	#include <string>
#endif

namespace RB
{
	constexpr wchar_t* LOGTAG_MAIN		= L"Main";
	constexpr wchar_t* LOGTAG_EVENT		= L"Event";
	constexpr wchar_t* LOGTAG_ENTITY	= L"Entity";
	constexpr wchar_t* LOGTAG_GRAPHICS	= L"Graphics";
	constexpr wchar_t* LOGTAG_WINDOWING = L"Windowing";
}

namespace RB::Utils::Debug
{
#ifdef RB_ENABLE_LOGS

	#ifdef RB_CORE_ACCESS
		#define RB_LOG(tag, ...)		{ RB::Utils::Debug::Logger::SetModeNormal();	RB::Utils::Debug::Logger::LogCore(tag, __VA_ARGS__); }	
		#define RB_LOG_WARN(tag, ...)	{ RB::Utils::Debug::Logger::SetModeWarn();		RB::Utils::Debug::Logger::LogCore(tag, __VA_ARGS__); }
		#define RB_LOG_ERROR(tag, ...)	{ RB::Utils::Debug::Logger::SetModeError();		RB::Utils::Debug::Logger::LogCore(tag, __VA_ARGS__); }
	#else
		#define RB_LOG(...)				{ RB::Utils::Debug::Logger::SetModeNormal();	RB::Utils::Debug::Logger::LogApp(__VA_ARGS__); }	
		#define RB_LOG_WARN(...)		{ RB::Utils::Debug::Logger::SetModeWarn();		RB::Utils::Debug::Logger::LogApp(__VA_ARGS__); }
		#define RB_LOG_ERROR(...)		{ RB::Utils::Debug::Logger::SetModeError();		RB::Utils::Debug::Logger::LogApp(__VA_ARGS__); }
	#endif

	#define RB_LOG_RELEASE(...)			RB_LOG(__VA_ARGS__)
	#define RB_LOG_CRITICAL(...)		RB_LOG_ERROR(__VA_ARGS__)

#else

	#define RB_LOG(...)
	#define RB_LOG_WARN(...)
	#define RB_LOG_ERROR(...)

	#define RB_LOG_RELEASE(tag, ...)	{ char msg[500]; sprintf(msg, __VA_ARGS__);  OutputDebugStringA(msg); OutputDebugStringA("\n"); }
	#define RB_LOG_CRITICAL(tag, ...)	RB_LOG_RELEASE(tag, "Critical error:") RB_LOG_RELEASE(tag, __VA_ARGS__)

#endif

#ifdef RB_ENABLE_LOGS
	namespace Logger
	{
		void OpenConsole();

		void SetModeNormal();
		void SetModeWarn();
		void SetModeError();

		void LogCore(const wchar_t* tag, const char* format, ...);
		void LogCore(const wchar_t* tag, const wchar_t* format, ...);
		void LogApp(const char* format, ...);
	}
#endif
}