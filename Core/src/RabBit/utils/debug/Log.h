#pragma once

#include "Core.h"

namespace RB::Debug
{
#ifdef RB_ENABLE_LOGS

	#ifdef RB_CORE_ACCESS
		#define RB_LOG(...)			{ RB::Debug::Logger::SetModeNormal();	RB::Debug::Logger::LogCore(__VA_ARGS__); }	
		#define RB_LOG_WARN(...)	{ RB::Debug::Logger::SetModeWarn();		RB::Debug::Logger::LogCore(__VA_ARGS__); }
		#define RB_LOG_ERROR(...)	{ RB::Debug::Logger::SetModeError();	RB::Debug::Logger::LogCore(__VA_ARGS__); }
	#else
		#define RB_LOG(...)			{ RB::Debug::Logger::SetModeNormal();	RB::Debug::Logger::LogApp(__VA_ARGS__); }	
		#define RB_LOG_WARN(...)	{ RB::Debug::Logger::SetModeWarn();		RB::Debug::Logger::LogApp(__VA_ARGS__); }
		#define RB_LOG_ERROR(...)	{ RB::Debug::Logger::SetModeError();	RB::Debug::Logger::LogApp(__VA_ARGS__); }
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