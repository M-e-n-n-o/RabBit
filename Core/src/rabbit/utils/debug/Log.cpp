#include "RabBitCommon.h"
#include "Log.h"

#ifdef RB_ENABLE_LOGS
namespace RB::Utils::Debug
{
    void Logger::OpenConsole()
    {
#if RB_PLATFORM_WINDOWS
        setlocale(LC_ALL, "");

        AllocConsole();
        int succeeded = freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

        DWORD dwMode = 0;
        if (!GetConsoleMode(console, &dwMode)) 
            return;

        // Enable colored output using ANSI escape codes
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(console, dwMode);
#endif
    }

    void Logger::SetModeNormal()
    {
        // Green
        printf("\033[1;32m");
    }

    void Logger::SetModeWarn()
    {
        // Orange/Yellow
        printf("\033[1;33m");
    }

    void Logger::SetModeError()
    {
        // Red
        printf("\033[1;31m");
    }

    void Logger::LogCore(const wchar_t* tag, const char* format, ...)
    {
        if (strlen(format) == 0)
        {
            wprintf(L"\n");
            return;
        }

        // TODO Also print out the timestamp

        va_list args;
        va_start(args, format);
        if (wcslen(tag) != 0)
            wprintf(L"[RabBit-%s] ", tag);
        vprintf(format, args);
        va_end(args);
    }

    void Logger::LogCore(const wchar_t* tag, const wchar_t* format, ...)
    {
        if (wcslen(format) == 0)
        {
            wprintf(L"\n");
            return;
        }

        va_list args;
        va_start(args, format);
        if (wcslen(tag) != 0)
            wprintf(L"[RabBit-%s] ", tag);
        vwprintf(format, args);
        va_end(args);
    }

    void Logger::LogApp(const char* format, ...)
    {
        if (strlen(format) == 0)
        {
            wprintf(L"\n");
            return;
        }

        va_list args;
        va_start(args, format);
        wprintf(L"[App] ");
        vprintf(format, args);
        va_end(args);
    }
}
#endif