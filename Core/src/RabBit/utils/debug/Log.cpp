#include "RabBitCommon.h"
#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef RB_ENABLE_LOGS
namespace RB::Utils::Debug
{
	void Logger::OpenConsole()
	{
		setlocale(LC_ALL, "");

		AllocConsole();
		int succeeded = freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	}

	void Logger::SetModeNormal()
	{
		// Green
		HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(console, 2);
	}

	void Logger::SetModeWarn()
	{
		// Orange
		HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(console, 6);
	}

	void Logger::SetModeError()
	{
		// Red
		HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(console, 4);
	}

	void Logger::LogCore(const wchar_t* tag, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		wprintf(L"[RabBit][%s] ", tag);
		vprintf(format, args);
		wprintf(L"\n");
		va_end(args);
	}

	void Logger::LogCore(const wchar_t* tag, const wchar_t* format, ...)
	{
		va_list args;
		va_start(args, format);
		wprintf(L"[RabBit][%s] ", tag);
		vwprintf(format, args);
		wprintf(L"\n");
		va_end(args);
	}

	void Logger::LogApp(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		wprintf(L"[App] ");
		vprintf(format, args);
		wprintf(L"\n");
		va_end(args);
	}
}
#endif