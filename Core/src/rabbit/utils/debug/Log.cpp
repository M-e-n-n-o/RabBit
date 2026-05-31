#include "RabBitCommon.h"
#include "Log.h"

#ifdef RB_ENABLE_LOGS

#include <iomanip>

namespace RB::Utils::Debug
{
    static int m_CurrentMode = Logger::kOutputMode_Normal;
    static void(*m_CustomOutput)(int, const char*, va_list) = nullptr;

    void Print(char const* const format, ...)
    {
        va_list args;
        va_start(args, format);

        if (m_CustomOutput)
            m_CustomOutput(m_CurrentMode, format, args);
        else
            vprintf(format, args);

        va_end(args);
    }

    void PrintV(char const* const format, va_list args)
    {
        if (m_CustomOutput)
            m_CustomOutput(m_CurrentMode, format, args);
        else
            vprintf(format, args);
    }

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
        m_CurrentMode = Logger::kOutputMode_Normal;

        if (!m_CustomOutput)
            printf("\033[1;32m"); // Green
    }

    void Logger::SetModeWarn()
    {
        m_CurrentMode = Logger::kOutputMode_Warn;

        if (!m_CustomOutput)
            printf("\033[1;33m"); // Orange/Yellow
    }

    void Logger::SetModeError()
    {
        m_CurrentMode = Logger::kOutputMode_Error;

        if (!m_CustomOutput)
            printf("\033[1;31m"); // Red
    }

    void Logger::LogTime()
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();

        Print("[%s] ", oss.str().c_str());
    }

    void Logger::LogCore(const char* tag, const char* format, ...)
    {
        if (strlen(format) == 0)
        {
            Print("\n");
            return;
        }

        va_list args;
        va_start(args, format);
        if (strlen(tag) != 0)
            Print("[RabBit-%s] ", tag);
        PrintV(format, args);
        va_end(args);
    }

    void Logger::LogApp(const char* format, ...)
    {
        if (strlen(format) == 0)
        {
            Print("\n");
            return;
        }

        va_list args;
        va_start(args, format);
        Print("[App] ");
        PrintV(format, args);
        va_end(args);
    }

    void Logger::SetCustomOutput(void(*CustomLog)(int, const char*, va_list))
    {
        if (CustomLog != nullptr && m_CustomOutput == nullptr)
        {
            m_CustomOutput = CustomLog;

            // Close console window
            fflush(stdout);
            fclose(stdout);
            FreeConsole();
        }
    }
}
#endif