#pragma once

#include "Core.h"

namespace RB
{
#ifdef RB_ENABLE_LOGS

	#ifdef RB_CORE_ACCESS
		#define RB_LOG(...)			{ RB::Logger::SetModeNormal();	RB::Logger::LogCore(__VA_ARGS__); }	
		#define RB_LOG_WARN(...)	{ RB::Logger::SetModeWarn();	RB::Logger::LogCore(__VA_ARGS__); }
		#define RB_LOG_ERROR(...)	{ RB::Logger::SetModeError();	RB::Logger::LogCore(__VA_ARGS__); }
	#else
		#define RB_LOG(...)			{ RB::Logger::SetModeNormal();	RB::Logger::LogApp(__VA_ARGS__); }	
		#define RB_LOG_WARN(...)	{ RB::Logger::SetModeWarn();	RB::Logger::LogApp(__VA_ARGS__); }
		#define RB_LOG_ERROR(...)	{ RB::Logger::SetModeError();	RB::Logger::LogApp(__VA_ARGS__); }
	#endif

	#define RB_LOG_RELEASE(...)		RB_LOG(__VA_ARGS__)
	#define RB_LOG_CRITICAL(...)	RB_LOG_ERROR(__VA_ARGS__)

#else

	#define RB_LOG(...)
	#define RB_LOG_WARN(...)
	#define RB_LOG_ERROR(...)

	#define RB_LOG_RELEASE(...) { char msg[500]; sprintf(msg, __VA_ARGS__);  OutputDebugStringA(msg); OutputDebugStringA("\n"); }
	#define RB_LOG_CRITICAL(...) RB_LOG_RELEASE("Critical error:") RB_LOG_RELEASE(__VA_ARGS__)

#endif

#ifdef RB_ENABLE_LOGS
	namespace Logger
	{
		void OpenConsole();

		void SetModeNormal();
		void SetModeWarn();
		void SetModeError();

		void LogCore(const char* format, ...);
		void LogApp(const char* format, ...);
	}
#endif
}