#include "RabBitCommon.h"
#include "Log.h"

#ifdef RB_ENABLE_LOGS

#include <iomanip>

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

    void Logger::LogTime()
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();

        printf("[%s] ", oss.str().c_str());
    }

    void Logger::LogCore(const char* tag, const char* format, ...)
    {
        if (strlen(format) == 0)
        {
            printf("\n");
            return;
        }

        va_list args;
        va_start(args, format);
        if (strlen(tag) != 0)
            printf("[RabBit-%s] ", tag);
        vprintf(format, args);
        va_end(args);
    }

    void Logger::LogApp(const char* format, ...)
    {
        if (strlen(format) == 0)
        {
            printf("\n");
            return;
        }

        va_list args;
        va_start(args, format);
        printf("[App] ");
        vprintf(format, args);
        va_end(args);
    }
}
#endif